#include <iostream>
#include <fstream>
#include <iomanip> 
#include <vector>
#include <windows.h>
#include <string>
#include <ctime>
#include <tlhelp32.h>
#include <tchar.h>
#include <nlohmann/json.hpp>
#include <cpr/cpr.h>

using namespace std;
using json = nlohmann::json;

// interval in seconds for how often variables are recorded
const int INTERVAL = 1;

HANDLE hProcHandle = NULL;

uintptr_t GetModuleBaseAddress(DWORD dwProcID, TCHAR *szModuleName);
void collectGameVars(HANDLE hProcHandle);
void commitVectors();
void resetVectors();
void writeLogFile();
void sendToServer();

string gameName = "Devil Daggers";
LPCSTR gameWindow = "Devil Daggers";
string gameStatus;
uintptr_t exeBaseAddress = 0;

bool isGameAvail;
bool updateOnNextRun;
bool recording = false;

int recordingCounter = 0;

// testing
int testSubmitCounter = 0;

// GAME VARS
float oldInGameTimer;
float inGameTimer;
DWORD inGameTimerBaseAddress = 0x001F30C0;
DWORD inGameTimerOffset = 0x1A0;
vector <float> inGameTimerVector;
int playerID;
DWORD playerIDBaseAddress = 0x001F30C0;
DWORD playerIDOffset = 0x5C;
int gems;
DWORD gemsBaseAddress = 0x001F30C0;
DWORD gemsOffset = 0x1C0;
vector <int> gemsVector;
int homingDaggers;
DWORD homingDaggersBaseAddress = 0x001F8084;
DWORD homingDaggersOffset = 0x224;
vector <int> homingDaggersVector;
int daggersFired;
DWORD daggersFiredBaseAddress = 0x001F30C0;
DWORD daggersFiredOffset = 0x1B4;
vector <int> daggersFiredVector;
int daggersHit;
DWORD daggersHitBaseAddress = 0x001F30C0;
DWORD daggersHitOffset = 0x1B8;
vector <int> daggersHitVector;
int enemiesAlive;
DWORD enemiesAliveBaseAddress = 0x001F30C0;
DWORD enemiesAliveOffset = 0x1FC;
vector <int> enemiesAliveVector;
int enemiesKilled;
DWORD enemiesKilledBaseAddress = 0x001F30C0;
DWORD enemiesKilledOffset = 0x1BC;
vector <int> enemiesKilledVector;
int deathType;
DWORD deathTypeBaseAddress = 0x001F30C0;
DWORD deathTypeOffset = 0x1C4;
int alive;
DWORD aliveBaseAddress = 0x001F30C0;
DWORD aliveOffset = 0x1A4;
bool isReplay;
DWORD isReplayBaseAddress = 0x001F30C0;
DWORD isReplayOffset = 0x35D;

// GEM VARS
bool gemStatus = false;
float gemOnScreenValue;

int main() {
	HWND hGameWindow = NULL;
	int timeSinceLastUpdate = clock();
	int gameAvailTimer = clock();
	int onePressTimer = clock();

	DWORD dwProcID = NULL;
	updateOnNextRun = true;
	string sGemStatus = "OFF";

	while (!GetAsyncKeyState(VK_F10)) {
		if (clock() - gameAvailTimer > 100) {
			gameAvailTimer = clock();
			isGameAvail = false;

			hGameWindow = FindWindow(NULL, gameWindow);
			if (hGameWindow) {
				GetWindowThreadProcessId(hGameWindow, &dwProcID);
				if (dwProcID) {
					hProcHandle = OpenProcess(PROCESS_ALL_ACCESS, FALSE, dwProcID);
					if (hProcHandle == INVALID_HANDLE_VALUE || hProcHandle == NULL) {
						gameStatus = "Failed to open process with valid handle.";
					} else {
						gameStatus = "Connected to Devil Daggers";
						exeBaseAddress = GetModuleBaseAddress(dwProcID, (_TCHAR*)_T("dd.exe"));
						isGameAvail = true;
					}
				} else {
					gameStatus = "Failed to get process ID.";
				}
			} else {
				gameStatus = "Devil Daggers not found.";
			}

			if (updateOnNextRun || clock() - timeSinceLastUpdate > 5000) {
				system("cls");
				cout << "------------------------------------------------------" << endl;
				cout << "                      ddstats.com" << endl;
				cout << "------------------------------------------------------" << endl << endl;
				cout << "Game Status: " << gameStatus << endl << endl;
				cout << "In Game Timer: " << inGameTimer << endl;
				cout << "Gem Count: " << gems << endl;
				cout << "Homing Dagger Count: " << homingDaggers << endl;
				cout << "Daggers Fired: " << daggersFired << endl;
				cout << "Daggers Hit: " << daggersHit << endl;
				if (daggersFired > 0.0)
					cout << "Accuracy: " << setprecision(4) << ((float) daggersHit / (float) daggersFired) * 100.0 << "%" << endl;
				else
					cout << "Accuracy: 0%" << endl;
				cout << "Enemies Alive: " << enemiesAlive << endl;
				cout << "Enemies Killed: " << enemiesKilled << endl;
				cout << "In Game Timer Vector Size: " << inGameTimerVector.size() << endl;
				cout << "Submissions: " << testSubmitCounter << endl;
				cout << endl << "[F10] Exit" << endl;
				updateOnNextRun = false;
				timeSinceLastUpdate = clock();

			}

			if (isGameAvail) {
				//writeToMemory(hProcHandle);
				collectGameVars(hProcHandle);
				if (alive && inGameTimer > 0 && !isReplay) {
					recording = true;
					if (recordingCounter < inGameTimer) {
						commitVectors();
					}
				}
				if (!alive && recording == true) {
					recording = false;
					// submit
					commitVectors(); // one last commit to make sure death time is accurate
					sendToServer();
					resetVectors(); // reset after submit
					testSubmitCounter++;
				}

			}

			//if (clock() - onePressTimer > 400) {
			//	if (isGameAvail) {
			//		if (GetAsyncKeyState(VK_F1)) {
			//			onePressTimer = clock();
			//			gemStatus = !gemStatus;
			//			updateOnNextRun = true;
			//			if (gemStatus) sGemStatus = "ON";
			//			else sGemStatus = "OFF";
			//		}
			//	}
			//}
		}
	}

	return ERROR_SUCCESS;
}

void commitVectors() {
	recordingCounter += INTERVAL;
	inGameTimerVector.push_back(inGameTimer);
	gemsVector.push_back(gems);
	homingDaggersVector.push_back(homingDaggers);
	daggersFiredVector.push_back(daggersFired);
	daggersHitVector.push_back(daggersHit);
	enemiesAliveVector.push_back(enemiesAlive);
	enemiesKilledVector.push_back(enemiesKilled);
}

void resetVectors() {
	recordingCounter = 0;
	inGameTimerVector.clear();
	gemsVector.clear();
	homingDaggersVector.clear();
	daggersFiredVector.clear();
	daggersHitVector.clear();
	enemiesAliveVector.clear();
	enemiesKilledVector.clear();
}

void collectGameVars(HANDLE hProcHandle) {
	DWORD pointer;
	DWORD pTemp;
	DWORD pointerAddr;

	// inGameTimer
	pointer = exeBaseAddress + inGameTimerBaseAddress;
	if (!ReadProcessMemory(hProcHandle, (LPCVOID)pointer, &pTemp, sizeof(pTemp), NULL)) {
		cout << "Failed to read address for in game timer." << endl;
	}
	else {
		pointerAddr = pTemp + inGameTimerOffset;
		oldInGameTimer = inGameTimer;
		ReadProcessMemory(hProcHandle, (LPCVOID)pointerAddr, &inGameTimer, sizeof(inGameTimer), NULL);
		if ((inGameTimer < oldInGameTimer) && recording) {
			// submit, but use oldInGameTimer var instead of new... then continue.
			deathType = -1; // did not die, so no death type.
			sendToServer();
			resetVectors(); // reset after submit
			testSubmitCounter++;
		}
	}
	// isReplay
	pointer = exeBaseAddress + isReplayBaseAddress;
	if (!ReadProcessMemory(hProcHandle, (LPCVOID)pointer, &pTemp, sizeof(pTemp), NULL)) {
		cout << "Failed to read address for alive." << endl;
	}
	else {
		pointerAddr = pTemp + isReplayOffset;
		ReadProcessMemory(hProcHandle, (LPCVOID)pointerAddr, &isReplay, sizeof(isReplay), NULL);
	}
	// alive
	pointer = exeBaseAddress + aliveBaseAddress;
	if (!ReadProcessMemory(hProcHandle, (LPCVOID)pointer, &pTemp, sizeof(pTemp), NULL)) {
		cout << "Failed to read address for alive." << endl;
	} else {
		pointerAddr = pTemp + aliveOffset;
		ReadProcessMemory(hProcHandle, (LPCVOID)pointerAddr, &alive, sizeof(alive), NULL);
	}
	// gems
	pointer = exeBaseAddress + gemsBaseAddress;
	if (!ReadProcessMemory(hProcHandle, (LPCVOID)pointer, &pTemp, sizeof(pTemp), NULL)) {
		cout << "Failed to read address for gem counter." << endl;
	} else {
		pointerAddr = pTemp + gemsOffset;
		ReadProcessMemory(hProcHandle, (LPCVOID)pointerAddr, &gems, sizeof(gems), NULL);
	}
	// daggersFired
	pointer = exeBaseAddress + daggersFiredBaseAddress;
	if (!ReadProcessMemory(hProcHandle, (LPCVOID)pointer, &pTemp, sizeof(pTemp), NULL)) {
		cout << "Failed to read address for daggers fired." << endl;
	} else {
		pointerAddr = pTemp + daggersFiredOffset;
		ReadProcessMemory(hProcHandle, (LPCVOID)pointerAddr, &daggersFired, sizeof(daggersFired), NULL);
	}
	// daggersHit
	pointer = exeBaseAddress + daggersHitBaseAddress;
	if (!ReadProcessMemory(hProcHandle, (LPCVOID)pointer, &pTemp, sizeof(pTemp), NULL)) {
		cout << "Failed to read address for daggers hit." << endl;
	}
	else {
		pointerAddr = pTemp + daggersHitOffset;
		ReadProcessMemory(hProcHandle, (LPCVOID)pointerAddr, &daggersHit, sizeof(daggersHit), NULL);
	}
	// enemiesKilled
	pointer = exeBaseAddress + enemiesKilledBaseAddress;
	if (!ReadProcessMemory(hProcHandle, (LPCVOID)pointer, &pTemp, sizeof(pTemp), NULL)) {
		cout << "Failed to read address for enemies killed." << endl;
	}
	else {
		pointerAddr = pTemp + enemiesKilledOffset;
		ReadProcessMemory(hProcHandle, (LPCVOID)pointerAddr, &enemiesKilled, sizeof(enemiesKilled), NULL);
	}
	// enemiesAlive
	pointer = exeBaseAddress + enemiesAliveBaseAddress;
	if (!ReadProcessMemory(hProcHandle, (LPCVOID)pointer, &pTemp, sizeof(pTemp), NULL)) {
		cout << "Failed to read address for enemies alive." << endl;
	}
	else {
		pointerAddr = pTemp + enemiesAliveOffset;
		ReadProcessMemory(hProcHandle, (LPCVOID)pointerAddr, &enemiesAlive, sizeof(enemiesAlive), NULL);
	}
	// deathType
	pointer = exeBaseAddress + deathTypeBaseAddress;
	if (!ReadProcessMemory(hProcHandle, (LPCVOID)pointer, &pTemp, sizeof(pTemp), NULL)) {
		cout << "Failed to read address for death type." << endl;
	}
	else {
		pointerAddr = pTemp + deathTypeOffset;
		ReadProcessMemory(hProcHandle, (LPCVOID)pointerAddr, &deathType, sizeof(deathType), NULL);
	}
}

uintptr_t GetModuleBaseAddress(DWORD dwProcID, TCHAR *szModuleName) {
	uintptr_t ModuleBaseAddress = 0;
	HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, dwProcID);
	if (hSnapshot != INVALID_HANDLE_VALUE) {
		MODULEENTRY32 ModuleEntry32;
		ModuleEntry32.dwSize = sizeof(MODULEENTRY32);
		if (Module32First(hSnapshot, &ModuleEntry32)) {
			do {
				if (_tcsicmp(ModuleEntry32.szModule, szModuleName) == 0) {
					ModuleBaseAddress = (uintptr_t)ModuleEntry32.modBaseAddr;
					break;
				}
			} while (Module32Next(hSnapshot, &ModuleEntry32));
		}
		CloseHandle(hSnapshot);
	}
	return ModuleBaseAddress;
}

void writeLogFile() {
	DWORD pointer;
	DWORD pTemp;
	DWORD pointerAddr;

	// get playerID
	pointer = exeBaseAddress + playerIDBaseAddress;
	if (!ReadProcessMemory(hProcHandle, (LPCVOID)pointer, &pTemp, sizeof(pTemp), NULL)) {
		cout << "Failed to read address for playerID." << endl;
	}
	else {
		pointerAddr = pTemp + playerIDOffset;
		ReadProcessMemory(hProcHandle, (LPCVOID)pointerAddr, &playerID, sizeof(playerID), NULL);
	}
	float accuracy;
	if (daggersFired > 0.0)
		accuracy = ((float)daggersHit / (float)daggersFired) * 100.0;
	else
		accuracy = 0.0;
	json log = {
		{"playerID", playerID},
		{"inGameTimer", inGameTimer},
		{"inGameTimerVector", inGameTimerVector},
		{"gems", gems},
		{"gemsVector", gemsVector},
		{"homingDaggers", homingDaggers},
		{"homingDaggersVector", homingDaggersVector},
		{"daggersFired", daggersFired},
		{"daggersFiredVector", daggersFiredVector},
		{"daggersHit", daggersHit},
		{"daggersHitVector", daggersHitVector},
		{"accuracy", accuracy},
		{"enemiesAlive", enemiesAlive},
		{"enemiesAliveVector", enemiesAliveVector},
		{"enemiesKilled", enemiesKilled},
		{"enemiesKilledVector", enemiesKilledVector},
		{"deathType", deathType}
	};
	ofstream o(to_string(clock()) + ".json");
	o << setw(4) << setprecision(4) << log << endl;
}

void sendToServer() {
	DWORD pointer;
	DWORD pTemp;
	DWORD pointerAddr;

	// get playerID
	pointer = exeBaseAddress + playerIDBaseAddress;
	if (!ReadProcessMemory(hProcHandle, (LPCVOID)pointer, &pTemp, sizeof(pTemp), NULL)) {
		cout << "Failed to read address for playerID." << endl;
	}
	else {
		pointerAddr = pTemp + playerIDOffset;
		ReadProcessMemory(hProcHandle, (LPCVOID)pointerAddr, &playerID, sizeof(playerID), NULL);
	}
	float accuracy;
	if (daggersFired > 0.0)
		accuracy = ((float)daggersHit / (float)daggersFired) * 100.0;
	else
		accuracy = 0.0;
	json log = {
		{ "playerID", playerID },
		{ "inGameTimer", inGameTimer },
		{ "inGameTimerVector", inGameTimerVector },
		{ "gems", gems },
		{ "gemsVector", gemsVector },
		{ "homingDaggers", homingDaggers },
		{ "homingDaggersVector", homingDaggersVector },
		{ "daggersFired", daggersFired },
		{ "daggersFiredVector", daggersFiredVector },
		{ "daggersHit", daggersHit },
		{ "daggersHitVector", daggersHitVector },
		{ "accuracy", accuracy },
		{ "enemiesAlive", enemiesAlive },
		{ "enemiesAliveVector", enemiesAliveVector },
		{ "enemiesKilled", enemiesKilled },
		{ "enemiesKilledVector", enemiesKilledVector },
		{ "deathType", deathType }
	};
	auto r = cpr::PostAsync(cpr::Url{ "http://10.0.1.222:5000/backend_score_submission" },
					   cpr::Body{ log.dump() },
					   cpr::Header{ {"Content-Type", "application/json"} });
}
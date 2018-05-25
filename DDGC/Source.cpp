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
int interval = 1;

HANDLE hProcHandle = NULL;

uintptr_t GetModuleBaseAddress(DWORD dwProcID, TCHAR *szModuleName);
void collectGameVars(HANDLE hProcHandle);
void commitVectors();
void resetVectors();
void writeLogFile();
future<cpr::Response> sendToServer();

string gameName = "Devil Daggers";
LPCSTR gameWindow = "Devil Daggers";
string gameStatus;
uintptr_t exeBaseAddress = 0;

bool isGameAvail;
bool updateOnNextRun;
bool recording = false;

int recordingCounter = 0;

// testing
int submitCounter = 0;
future<cpr::Response> future_response = future<cpr::Response>{};
string errorLine = "";
json jsonResponse;

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
bool alive;
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
				cout << "                      ddstats" << endl;
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
				if (future_response.valid()) {
					if (future_response.wait_for(std::chrono::seconds(0)) == std::future_status::ready) {
						auto r = future_response.get();
						if (r.status_code >= 400 || r.status_code == 0) {
							errorLine = "Error [" + to_string(r.status_code) + "] submitting score.";
							jsonResponse = json();
						} else {
							jsonResponse = json::parse(r.text);
							errorLine = "";
							submitCounter++;
						}
						future_response = future<cpr::Response>{};
					}
				}
				cout << "Submissions: " << submitCounter << endl;
				if (errorLine != "") {
					std::cout << std::endl << errorLine << std::endl;
				}
				if (!jsonResponse.empty()) {
					cout << std::endl << jsonResponse.at("message").get<std::string>() << endl;
				}
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
					// this is to assure the correct last entry when user dies
					if (!homingDaggersVector.empty())
						homingDaggers = homingDaggersVector.back();
					// submit
					commitVectors(); // one last commit to make sure death time is accurate
					future_response = sendToServer();
					resetVectors(); // reset after submit
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
		Sleep(15); // cut back on some(?) overhead
	}

	return ERROR_SUCCESS;
}

void commitVectors() {
	recordingCounter += interval;
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
			future_response = sendToServer();
			resetVectors(); // reset after submit
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
	// homingDaggers
	pointer = exeBaseAddress + homingDaggersBaseAddress;
	if (!ReadProcessMemory(hProcHandle, (LPCVOID)pointer, &pTemp, sizeof(pTemp), NULL)) {
		cout << "Failed to read address for homing daggers." << endl;
	}
	else {
		// 2 pointer offsets for homingDaggers
		pointerAddr = pTemp + 0x0;
		ReadProcessMemory(hProcHandle, (LPCVOID)pointerAddr, &pTemp, sizeof(pTemp), NULL);
		pointerAddr = pTemp + homingDaggersOffset;
		ReadProcessMemory(hProcHandle, (LPCVOID)pointerAddr, &homingDaggers, sizeof(homingDaggers), NULL);
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
	//float accuracy;
	//if (daggersFired > 0.0)
	//	accuracy = ((float)daggersHit / (float)daggersFired) * 100.0;
	//else
	//	accuracy = 0.0;
	json log = {
		{"playerID", playerID},
		{"granularity", interval},
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
		// {"accuracy", accuracy},
		{"enemiesAlive", enemiesAlive},
		{"enemiesAliveVector", enemiesAliveVector},
		{"enemiesKilled", enemiesKilled},
		{"enemiesKilledVector", enemiesKilledVector},
		{"deathType", deathType}
	};
	ofstream o(to_string(clock()) + ".json");
	o << setw(4) << setprecision(4) << log << endl;
}

std::future<cpr::Response> sendToServer() {
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
	//float accuracy;
	//if (daggersFired > 0.0)
	//	accuracy = ((float)daggersHit / (float)daggersFired) * 100.0;
	//else
	//	accuracy = 0.0;
	json log = {
		{ "playerID", playerID },
		{ "granularity", interval },
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
		// { "accuracy", accuracy },
		{ "enemiesAlive", enemiesAlive },
		{ "enemiesAliveVector", enemiesAliveVector },
		{ "enemiesKilled", enemiesKilled },
		{ "enemiesKilledVector", enemiesKilledVector },
		{ "deathType", deathType }
	};
	// auto r = cpr::PostAsync(cpr::Url{ "http://www.ddstats.com/api/submit_game" },
	//auto r = cpr::PostAsync(cpr::Url{ "http://10.0.1.222:5666/submit_game" },
					  // cpr::Body{ log.dump() },
					//   cpr::Header{ {"Content-Type", "application/json"} });


	auto future_response = cpr::PostCallback([](cpr::Response r) {
		return r;
	}, 
		cpr::Url{ "http://ddstats.com/api/submit_game" },
		cpr::Body{ log.dump() }, 
		cpr::Header{ { "Content-Type", "application/json" } });

	//pending_futures.push_back(std::move(future_text));
	//// Sometime later
	//if (future_text.wait_for(chrono::seconds(0)) == future_status::ready) {
	//	cout << future_text.get() << endl;
	//}

	return future_response;

}
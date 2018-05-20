#include <iostream>
#include <windows.h>
#include <string>
#include <ctime>
#include <tlhelp32.h>
#include <tchar.h>

using namespace std;

HANDLE hProcHandle = NULL;

uintptr_t GetModuleBaseAddress(DWORD dwProcID, TCHAR *szModuleName);
// DWORD findDmaAddy(int pointerLevel, HANDLE hProcHandle, DWORD offsets[], DWORD BaseAddress);
void writeToMemory(HANDLE hProcHandle);
void collectGameVars(HANDLE hProcHandle);

string gameName = "Devil Daggers";
LPCSTR gameWindow = "Devil Daggers";
string gameStatus;
uintptr_t exeBaseAddress = 0;

bool isGameAvail;
bool updateOnNextRun;
bool recording = false;

// testing
int testSubmitCounter = 0;

// GAME VARS
float oldInGameTimer;
float inGameTimer;
DWORD inGameTimerBaseAddress = 0x001F30C0;
DWORD inGameTimerOffset = 0x1A0;
int gems;
DWORD gemsBaseAddress = 0x001F30C0;
DWORD gemsOffset = 0x1C0;
int homingDaggers;
DWORD homingDaggersBaseAddress = 0x001F8084;
DWORD homingDaggersOffset = 0x224;
int daggersFired;
DWORD daggersFiredBaseAddress = 0x001F30C0;
DWORD daggersFiredOffset = 0x1B4;
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
				cout << "[F1] Monitor -> " << sGemStatus << " <-" << endl;
				cout << "In Game Timer: " << inGameTimer << endl;
				cout << "Gem Count: " << gems << endl;
				cout << "Homing Dagger Count: " << homingDaggers << endl;
				cout << "Daggers Fired: " << daggersFired << endl;
				cout << "Submissions: " << testSubmitCounter << endl;
				cout << "[F10] Exit" << endl;
				updateOnNextRun = false;
				timeSinceLastUpdate = clock();

			}

			if (isGameAvail) {
				//writeToMemory(hProcHandle);
				collectGameVars(hProcHandle);
				if (alive && inGameTimer > 0 && !isReplay) {
					recording = true;
				}
				if (!alive && recording == true) {
					recording = false;
					// submit
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

//DWORD findDmaAddy(int pointerLevel, HANDLE hProcHandle, DWORD offsets[], DWORD BaseAddress) {
//	DWORD pointer = BaseAddress;
//	DWORD pTemp;
//
//	DWORD pointerAddr;
//	for (int i = 0; i < pointerLevel; i++) {
//		if (i == 0) {
//			ReadProcessMemory(hProcHandle, (LPCVOID)pointer, &pTemp, sizeof(pTemp), NULL);
//		}
//
//		pointerAddr = pTemp + offsets[i];
//		ReadProcessMemory(hProcHandle, (LPCVOID)pointerAddr, &pTemp, sizeof(pTemp), NULL);
//		cout << "test" << endl;
//	}
//	return pointerAddr;
//}

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
}


void writeToMemory(HANDLE hProcHandle) {
	DWORD pointer = exeBaseAddress + gemsBaseAddress;
	DWORD pTemp;

	DWORD pointerAddr;
	if (gemStatus) {
		// addressToWrite = findDmaAddy(1, hProcHandle, gemOffset, gemBaseAddress);
		if (!ReadProcessMemory(hProcHandle, (LPCVOID)pointer, &pTemp, sizeof(pTemp), NULL))
			cout << "Failed to read address for gem counter." << endl;

		pointerAddr = pTemp + 0x1C0;
		ReadProcessMemory(hProcHandle, (LPCVOID)pointerAddr, &gems, sizeof(gems), NULL);
		gemOnScreenValue = gems;

		if (!ReadProcessMemory(hProcHandle, (LPCVOID)pointer, &pTemp, sizeof(pTemp), NULL))
			cout << "Failed to read address for player best." << endl;

		string testing = "LukeNukemRR";
		SIZE_T sizeOfTesting = testing.length() + 1;
		pointerAddr = pTemp + 0x30C;

		//if (!WriteProcessMemory(hProcHandle, (BYTE*)pointerAddr, &gemOnScreenValue, sizeof(gemOnScreenValue), NULL))
		//	cout << "Failed to write process memory." << endl;
		if (!WriteProcessMemory(hProcHandle, (BYTE*)pointerAddr, testing.c_str(), testing.length()+1, NULL))
			cout << "Failed to write process memory." << endl;
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
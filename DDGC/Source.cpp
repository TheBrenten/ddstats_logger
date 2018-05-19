#include <iostream>
#include <windows.h>
#include <string>
#include <ctime>
#include <tlhelp32.h>
#include <tchar.h>

using namespace std;

HANDLE hProcHandle = NULL;

uintptr_t GetModuleBaseAddress(DWORD dwProcID, TCHAR *szModuleName);
DWORD findDmaAddy(int pointerLevel, HANDLE hProcHandle, DWORD offsets[], DWORD BaseAddress);
void writeToMemory(HANDLE hProcHandle);
void collectGameVars(HANDLE hProcHandle);

string gameName = "Devil Daggers";
LPCSTR gameWindow = "Devil Daggers";
string gameStatus;
uintptr_t exeBaseAddress = 0;

bool isGameAvail;
bool updateOnNextRun;

// GAME VARS
float inGameTimer;
DWORD inGameTimerBaseAddress = 0x001F30C0;
DWORD inGameTimerOffset = 0x1A0;

// GEM VARS
bool gemStatus = false;
int gemValue;
float gemOnScreenValue;
DWORD gemBaseAddress = 0x001F30C0;
DWORD gemOffset = 0x1C0;

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
				cout << "[F1] Toggle Gem Counter -> " << sGemStatus << " <-" << to_string(gemValue) << endl;
				cout << "In Game Timer: " << to_string(inGameTimer) << endl;
				cout << "[F10] Exit" << endl;
				updateOnNextRun = false;
				timeSinceLastUpdate = clock();

			}

			if (isGameAvail) {
				//writeToMemory(hProcHandle);
				collectGameVars(hProcHandle);
			}

			if (clock() - onePressTimer > 400) {
				if (isGameAvail) {
					if (GetAsyncKeyState(VK_F1)) {
						onePressTimer = clock();
						gemStatus = !gemStatus;
						updateOnNextRun = true;
						if (gemStatus) sGemStatus = "ON";
						else sGemStatus = "OFF";
					}
				}
			}
		}
	}

	return ERROR_SUCCESS;
}

DWORD findDmaAddy(int pointerLevel, HANDLE hProcHandle, DWORD offsets[], DWORD BaseAddress) {
	DWORD pointer = BaseAddress;
	DWORD pTemp;

	DWORD pointerAddr;
	for (int i = 0; i < pointerLevel; i++) {
		if (i == 0) {
			ReadProcessMemory(hProcHandle, (LPCVOID)pointer, &pTemp, sizeof(pTemp), NULL);
		}

		pointerAddr = pTemp + offsets[i];
		ReadProcessMemory(hProcHandle, (LPCVOID)pointerAddr, &pTemp, sizeof(pTemp), NULL);
		cout << "test" << endl;
	}
	return pointerAddr;
}

void readPointer(DWORD baseAddress, DWORD offset) {
	DWORD pointer = exeBaseAddress + baseAddress;
}


void collectGameVars(HANDLE hProcHandle) {
	DWORD pointer;
	DWORD pTemp;
	DWORD pointerAddr;

	if (gemStatus) {
		// gems
		pointer = exeBaseAddress + gemBaseAddress;
		if (!ReadProcessMemory(hProcHandle, (LPCVOID)pointer, &pTemp, sizeof(pTemp), NULL))
			cout << "Failed to read address for gem counter." << endl;
		pointerAddr = pTemp + gemOffset;
		ReadProcessMemory(hProcHandle, (LPCVOID)pointerAddr, &gemValue, sizeof(gemValue), NULL);
		// time
		pointer = exeBaseAddress + inGameTimerBaseAddress;
		if (!ReadProcessMemory(hProcHandle, (LPCVOID)pointer, &pTemp, sizeof(pTemp), NULL))
			cout << "Failed to read address for in game timer." << endl;
		pointerAddr = pTemp + inGameTimerOffset;
		ReadProcessMemory(hProcHandle, (LPCVOID)pointerAddr, &inGameTimer, sizeof(inGameTimer), NULL);
	}
}


void writeToMemory(HANDLE hProcHandle) {
	DWORD pointer = exeBaseAddress + gemBaseAddress;
	DWORD pTemp;

	DWORD pointerAddr;
	if (gemStatus) {
		// addressToWrite = findDmaAddy(1, hProcHandle, gemOffset, gemBaseAddress);
		if (!ReadProcessMemory(hProcHandle, (LPCVOID)pointer, &pTemp, sizeof(pTemp), NULL))
			cout << "Failed to read address for gem counter." << endl;

		pointerAddr = pTemp + 0x1C0;
		ReadProcessMemory(hProcHandle, (LPCVOID)pointerAddr, &gemValue, sizeof(gemValue), NULL);
		gemOnScreenValue = gemValue;

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
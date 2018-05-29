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
#include <curses.h>

using namespace std;
using json = nlohmann::json;

// current version
std::string version = "0.2.0";
bool updateAvailable = false;
bool validVersion = true;

// interval in seconds for how often variables are recorded
int interval = 1;

HANDLE hProcHandle = NULL;

uintptr_t GetModuleBaseAddress(DWORD dwProcID, TCHAR *szModuleName);
void collectGameVars(HANDLE hProcHandle);
void commitVectors();
void resetVectors();
// void writeLogFile();
future<cpr::Response> sendToServer();
future<cpr::Response> getMOTD();
void printTitle(WINDOW *win);
void printStats();

float inGameTimerSubmit = 0.0;
int gemsSubmit = 0;
int homingDaggersSubmit = 0;
int daggersFiredSubmit = 0;
int daggersHitSubmit = 0;
int enemiesAliveSubmit = 0;
int enemiesKilledSubmit = 0;
float accuracySubmit = 0.0;

int coln;
int leftAlign, rightAlign, centerAlign;

string gameName = "Devil Daggers";
LPCSTR gameWindow = "Devil Daggers";
string gameStatus;
uintptr_t exeBaseAddress = 0;

bool isGameAvail;
bool updateOnNextRun;
bool recording = false;
bool monitorStats = false;

int recordingCounter = 0;
std::string motd = "";

// testing
int submitCounter = 0;
future<cpr::Response> future_response = future<cpr::Response>{};
future<cpr::Response> future_motd_response = future<cpr::Response>{};
double elapsed;
string errorLine = "";
std::string lastGameSubmission;
std::string recordingStatus;
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

	// set up curses
	WINDOW *stdscr = initscr();
	raw();
	keypad(stdscr, TRUE);
	noecho();
	start_color();
	curs_set(0);
	// nodelay();

	int row, col;
	getmaxyx(stdscr, row, col);

	coln = 12;
	leftAlign = (col - 66) / 2;
	rightAlign = leftAlign + 66;
	centerAlign = col / 2;

	// color setup
	//init_pair(1, 53, -1);
	init_pair(1, COLOR_RED, COLOR_BLACK);
	init_pair(2, COLOR_GREEN, COLOR_BLACK);

	//printw("Type any character to see it in bold\n");
	//ch = getch();
	//if (ch == KEY_F(1))
	//	printw("F1 Key pressed");
	//else {
	//	printw("The pressed key is ");
	//	attron(A_BOLD);
	//	printw("%c", ch);
	//	attroff(A_BOLD);
	//}
	//refresh();
	//getch();
	// endwin();

	// return 0;

	// this sets up the colors for the console
	//HANDLE hConsole;
	//hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
	//SetConsoleTextAttribute(hConsole, 832);

	HWND hGameWindow = NULL;
	int timeSinceLastUpdate = clock();
	int gameAvailTimer = clock();
	int onePressTimer = clock();

	DWORD dwProcID = NULL;
	updateOnNextRun = true;
	// string sGemStatus = "OFF";

	future_motd_response = getMOTD();

	while (!GetAsyncKeyState(VK_F10)) {
		if ((clock() - gameAvailTimer > 100) && validVersion) {
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
						gameStatus = "[[ Connected to Devil Daggers ]]";
						exeBaseAddress = GetModuleBaseAddress(dwProcID, (_TCHAR*)_T("dd.exe"));
						isGameAvail = true;
					}
				} else {
					gameStatus = "Failed to get process ID.";
				}
			} else {
				gameStatus = "[[ Devil Daggers not found ]]";
			}

			if (updateOnNextRun || clock() - timeSinceLastUpdate > 500) {
				clear();
				coln = 12;

				printTitle(stdscr);

				if (future_motd_response.valid()) {
					if (future_motd_response.wait_for(std::chrono::seconds(0)) == std::future_status::ready) {
						auto r = future_motd_response.get();
						if (r.status_code >= 400 || r.status_code == 0) {
							motd = "Error [" + to_string(r.status_code) + "] connecting to ddstats.com.";
						} else {
							motd = json::parse(r.text).at("motd").get<std::string>();
							validVersion = json::parse(r.text).at("valid_version").get<bool>();
							updateAvailable = json::parse(r.text).at("update_available").get<bool>();
						}
					}
				}
				if (updateAvailable) {
					attron(COLOR_PAIR(2));
					mvprintw(coln-1, leftAlign, "(UPDATE AVAILABLE)");
					attroff(COLOR_PAIR(2));
				}
				mvprintw(coln, (col - motd.length()) / 2, "%s", motd.c_str());
				coln++;
				if (isGameAvail)
					attron(COLOR_PAIR(2));
				else
					attron(COLOR_PAIR(1));
				mvprintw(coln, (col - gameStatus.length()) / 2, gameStatus.c_str());
				if (isGameAvail)
					attroff(COLOR_PAIR(2));
				else
					attroff(COLOR_PAIR(1));
				coln++;
				if (recording && (inGameTimer != 0.0)) {
					attron(COLOR_PAIR(2));
					attron(A_BOLD);
					recordingStatus = " [[ Recording ]] ";
				} else {
					attron(COLOR_PAIR(1));
					recordingStatus = "[[ Not Recording ]]";
				}
				mvprintw(coln, (col - recordingStatus.length()) / 2, recordingStatus.c_str());
				if (recording && (inGameTimer != 0.0)) {
					attroff(COLOR_PAIR(2));
					attroff(A_BOLD);
				} else
					attroff(COLOR_PAIR(1));
				coln++;

				if (monitorStats) {
					// left column
					mvprintw(coln, leftAlign, "In Game Timer: %.4fs", inGameTimer);
					coln++;
					mvprintw(coln, leftAlign, "Daggers Hit: %d", daggersHit);
					coln++;
					mvprintw(coln, leftAlign, "Daggers Fired: %d", daggersFired);
					coln++;
					if (daggersFired > 0.0)
						mvprintw(coln, leftAlign, "Accuracy: %.2f%%", ((float)daggersHit / (float)daggersFired) * 100.0);
					else
						mvprintw(coln, leftAlign, "Accuracy: 0.00%%", daggersFired);

					// right column
					coln -= 3;
					mvprintw(coln, rightAlign-(6+to_string(gems).length()), "Gems: %d", gems);
					coln++;
					mvprintw(coln, rightAlign-(16+to_string(homingDaggers).length()), "Homing Daggers: %d", homingDaggers);
					coln++;
					// fix for title screen
					if (inGameTimer == 0.0)
						mvprintw(coln, rightAlign - 16, "Enemies Alive: %d", 0);
					else
						mvprintw(coln, rightAlign - (15 + to_string(enemiesAlive).length()), "Enemies Alive: %d", enemiesAlive);
					coln++;
					if (inGameTimer = 0.0)
						mvprintw(coln, rightAlign - 17, "Enemies Killed: %d", 0);
					else
						mvprintw(coln, rightAlign-(16+to_string(enemiesKilled).length()), "Enemies Killed: %d", enemiesKilled);
					coln++;
				} else {
					printStats();
				}

				if (future_response.valid()) {
					if (future_response.wait_for(std::chrono::seconds(0)) == std::future_status::ready) {
						auto r = future_response.get();
						if (r.status_code >= 400 || r.status_code == 0) {
							errorLine = "Error [" + to_string(r.status_code) + "] submitting run.";
							jsonResponse = json();
						} else {
							jsonResponse = json::parse(r.text);
							elapsed = r.elapsed;
							errorLine = "";
							submitCounter++;
						}
						future_response = future<cpr::Response>{};
					}
				}
				coln++;
				mvprintw(coln, leftAlign, "Last Submission:");
				if (errorLine != "") {
					attron(COLOR_PAIR(1));
					mvprintw(coln, leftAlign + 17, errorLine.c_str());
					attroff(COLOR_PAIR(1));
				} else if (!jsonResponse.empty()) {
					attron(COLOR_PAIR(2));
					mvprintw(coln, leftAlign + 17, "https://ddstats.com/game_log/%s", to_string(jsonResponse.at("game_id").get<std::int32_t>()).c_str());
					attroff(COLOR_PAIR(2));
				} else {
					mvprintw(coln, leftAlign + 17, "None, yet.");
				}
				coln++;
				mvprintw(coln, leftAlign, "[F10] Exit");

				refresh();

				updateOnNextRun = false;
				timeSinceLastUpdate = clock();
			}

			//if (updateOnNextRun || clock() - timeSinceLastUpdate > 5000) {
			//	system("cls");
			//	//cout << "------------------------------------------------------" << endl;
			//	//cout << "                      ddstats" << endl;
			//	printTitle();
			//	//cout << "------------------------------------------------------" << endl << endl;
			//	cout << " Game Status: " << gameStatus << endl << endl;
			//	cout << " In Game Timer: " << inGameTimer << endl;
			//	cout << " Gem Count: " << gems << endl;
			//	cout << " Homing Dagger Count: " << homingDaggers << endl;
			//	cout << " Daggers Fired: " << daggersFired << endl;
			//	cout << " Daggers Hit: " << daggersHit << endl;
			//	if (daggersFired > 0.0)
			//		cout << " Accuracy: " << setprecision(4) << ((float) daggersHit / (float) daggersFired) * 100.0 << "%" << endl;
			//	else
			//		cout << " Accuracy: 0%" << endl;
			//	cout << " Enemies Alive: " << enemiesAlive << endl;
			//	cout << " Enemies Killed: " << enemiesKilled << endl;
			//	if (future_response.valid()) {
			//		if (future_response.wait_for(std::chrono::seconds(0)) == std::future_status::ready) {
			//			auto r = future_response.get();
			//			if (r.status_code >= 400 || r.status_code == 0) {
			//				errorLine = " Error [" + to_string(r.status_code) + "] submitting run.";
			//				jsonResponse = json();
			//			} else {
			//				jsonResponse = json::parse(r.text);
			//				elapsed = r.elapsed;
			//				errorLine = "";
			//				submitCounter++;
			//			}
			//			future_response = future<cpr::Response>{};
			//		}
			//	}
			//	cout << " Submissions: " << submitCounter << endl;
			//	if (errorLine != "") {
			//		std::cout << std::endl << errorLine << std::endl;
			//	}
			//	if (!jsonResponse.empty()) {
			//		std::cout << std::endl << " Game submitted successfully in " << elapsed << " seconds!" << std::endl;
			//		std::cout << " You can access your game at:" << std::endl;
			//		std::cout << " https://ddstats.com/api/game/" <<
			//			jsonResponse.at("game_id").get<std::int32_t>() << std::endl;
			//	}
			//	cout << endl << " [F10] Exit" << endl;
			//	updateOnNextRun = false;
			//	timeSinceLastUpdate = clock();

			//}



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

			if (clock() - onePressTimer > 400) {
				if (isGameAvail) {
					if (GetAsyncKeyState(VK_F11)) {
						onePressTimer = clock();
						monitorStats = !monitorStats;
						updateOnNextRun = true;
					}
				}
			}
		}
		if (!validVersion) {	
			clear();
			mvprintw(0, leftAlign, "This version of ddstats.exe is no longer valid.");
			mvprintw(1, leftAlign, "Press any key to exit.");
			refresh();
			getch();
			endwin();
			return ERROR;
		}
		Sleep(15); // cut back on some(?) overhead
	}

	endwin();
	return ERROR_SUCCESS;
}

void printStats() {

	// left column
	mvprintw(coln, leftAlign, "In Game Timer: %.4fs", inGameTimerSubmit);
	coln++;
	mvprintw(coln, leftAlign, "Daggers Hit: %d", daggersHitSubmit);
	coln++;
	mvprintw(coln, leftAlign, "Daggers Fired: %d", daggersFiredSubmit);
	coln++;
	mvprintw(coln, leftAlign, "Accuracy: %.2f%%", accuracySubmit);

	// right column
	coln -= 3;
	mvprintw(coln, rightAlign - (6 + to_string(gemsSubmit).length()), "Gems: %d", gemsSubmit);
	coln++;
	mvprintw(coln, rightAlign - (16 + to_string(homingDaggersSubmit).length()), "Homing Daggers: %d", homingDaggersSubmit);
	coln++;
	mvprintw(coln, rightAlign - (15 + to_string(enemiesAliveSubmit).length()), "Enemies Alive: %d", enemiesAliveSubmit);
	coln++;
	mvprintw(coln, rightAlign - (16 + to_string(enemiesKilledSubmit).length()), "Enemies Killed: %d", enemiesKilledSubmit);
	coln++;

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
		cout << " Failed to read address for in game timer." << endl;
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
		cout << " Failed to read address for alive." << endl;
	}
	else {
		pointerAddr = pTemp + isReplayOffset;
		ReadProcessMemory(hProcHandle, (LPCVOID)pointerAddr, &isReplay, sizeof(isReplay), NULL);
	}
	// alive
	pointer = exeBaseAddress + aliveBaseAddress;
	if (!ReadProcessMemory(hProcHandle, (LPCVOID)pointer, &pTemp, sizeof(pTemp), NULL)) {
		cout << " Failed to read address for alive." << endl;
	} else {
		pointerAddr = pTemp + aliveOffset;
		ReadProcessMemory(hProcHandle, (LPCVOID)pointerAddr, &alive, sizeof(alive), NULL);
	}
	// gems
	pointer = exeBaseAddress + gemsBaseAddress;
	if (!ReadProcessMemory(hProcHandle, (LPCVOID)pointer, &pTemp, sizeof(pTemp), NULL)) {
		cout << " Failed to read address for gem counter." << endl;
	} else {
		pointerAddr = pTemp + gemsOffset;
		ReadProcessMemory(hProcHandle, (LPCVOID)pointerAddr, &gems, sizeof(gems), NULL);
	}
	// homingDaggers
	pointer = exeBaseAddress + homingDaggersBaseAddress;
	if (!ReadProcessMemory(hProcHandle, (LPCVOID)pointer, &pTemp, sizeof(pTemp), NULL)) {
		cout << " Failed to read address for homing daggers." << endl;
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
		cout << " Failed to read address for daggers fired." << endl;
	} else {
		pointerAddr = pTemp + daggersFiredOffset;
		ReadProcessMemory(hProcHandle, (LPCVOID)pointerAddr, &daggersFired, sizeof(daggersFired), NULL);
	}
	// daggersHit
	pointer = exeBaseAddress + daggersHitBaseAddress;
	if (!ReadProcessMemory(hProcHandle, (LPCVOID)pointer, &pTemp, sizeof(pTemp), NULL)) {
		cout << " Failed to read address for daggers hit." << endl;
	}
	else {
		pointerAddr = pTemp + daggersHitOffset;
		ReadProcessMemory(hProcHandle, (LPCVOID)pointerAddr, &daggersHit, sizeof(daggersHit), NULL);
	}
	// enemiesKilled
	pointer = exeBaseAddress + enemiesKilledBaseAddress;
	if (!ReadProcessMemory(hProcHandle, (LPCVOID)pointer, &pTemp, sizeof(pTemp), NULL)) {
		cout << " Failed to read address for enemies killed." << endl;
	}
	else {
		pointerAddr = pTemp + enemiesKilledOffset;
		ReadProcessMemory(hProcHandle, (LPCVOID)pointerAddr, &enemiesKilled, sizeof(enemiesKilled), NULL);
	}
	// enemiesAlive
	pointer = exeBaseAddress + enemiesAliveBaseAddress;
	if (!ReadProcessMemory(hProcHandle, (LPCVOID)pointer, &pTemp, sizeof(pTemp), NULL)) {
		cout << " Failed to read address for enemies alive." << endl;
	}
	else {
		pointerAddr = pTemp + enemiesAliveOffset;
		ReadProcessMemory(hProcHandle, (LPCVOID)pointerAddr, &enemiesAlive, sizeof(enemiesAlive), NULL);
	}
	// deathType
	pointer = exeBaseAddress + deathTypeBaseAddress;
	if (!ReadProcessMemory(hProcHandle, (LPCVOID)pointer, &pTemp, sizeof(pTemp), NULL)) {
		cout << " Failed to read address for death type." << endl;
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

//void writeLogFile() {
//	DWORD pointer;
//	DWORD pTemp;
//	DWORD pointerAddr;
//
//	// get playerID
//	pointer = exeBaseAddress + playerIDBaseAddress;
//	if (!ReadProcessMemory(hProcHandle, (LPCVOID)pointer, &pTemp, sizeof(pTemp), NULL)) {
//		cout << " Failed to read address for playerID." << endl;
//	}
//	else {
//		pointerAddr = pTemp + playerIDOffset;
//		ReadProcessMemory(hProcHandle, (LPCVOID)pointerAddr, &playerID, sizeof(playerID), NULL);
//	}
//	//float accuracy;
//	//if (daggersFired > 0.0)
//	//	accuracy = ((float)daggersHit / (float)daggersFired) * 100.0;
//	//else
//	//	accuracy = 0.0;
//	json log = {
//		{"playerID", playerID},
//		{"granularity", interval},
//		{"inGameTimer", inGameTimer},
//		{"inGameTimerVector", inGameTimerVector},
//		{"gems", gems},
//		{"gemsVector", gemsVector},
//		{"homingDaggers", homingDaggers},
//		{"homingDaggersVector", homingDaggersVector},
//		{"daggersFired", daggersFired},
//		{"daggersFiredVector", daggersFiredVector},
//		{"daggersHit", daggersHit},
//		{"daggersHitVector", daggersHitVector},
//		// {"accuracy", accuracy},
//		{"enemiesAlive", enemiesAlive},
//		{"enemiesAliveVector", enemiesAliveVector},
//		{"enemiesKilled", enemiesKilled},
//		{"enemiesKilledVector", enemiesKilledVector},
//		{"deathType", deathType}
//	};
//	ofstream o(to_string(clock()) + ".json");
//	o << setw(4) << setprecision(4) << log << endl;
//}

std::future<cpr::Response> sendToServer() {
	DWORD pointer;
	DWORD pTemp;
	DWORD pointerAddr;

	// get playerID
	pointer = exeBaseAddress + playerIDBaseAddress;
	if (!ReadProcessMemory(hProcHandle, (LPCVOID)pointer, &pTemp, sizeof(pTemp), NULL)) {
		cout << " Failed to read address for playerID." << endl;
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

	// this fixes the last captured time if user restarts to be accurate
	float inGameTimerFix;
	if (deathType == -1) {
		inGameTimerFix = oldInGameTimer;
	} else {
		inGameTimerFix = inGameTimer;
	}

	json log = {
		{ "playerID", playerID },
		{ "granularity", interval },
		{ "inGameTimer", inGameTimerFix },
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

	inGameTimerSubmit = inGameTimerFix;
	gemsSubmit = gems;
	homingDaggersSubmit = homingDaggers;
	daggersFiredSubmit = daggersFired;
	daggersHitSubmit = daggersHit;
	enemiesAliveSubmit = enemiesAlive;
	enemiesKilledSubmit = enemiesKilled;
	if (daggersFired == 0)
		accuracySubmit = 0.0;
	else
		accuracySubmit = (daggersHit / daggersFired) * 100.0;

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

std::future<cpr::Response> getMOTD() {
	auto future_response = cpr::PostCallback([](cpr::Response r) {
		return r;
	},
		cpr::Url{ "http://ddstats.com/api/get_motd" },
		cpr::Body{ "{ \"version\": \"" + version + "\" }" },
		cpr::Header{ { "Content-Type", "application/json" } });
	return future_response;
}

void printTitle(WINDOW *win) {

	int row, col;

	getmaxyx(win, row, col);

	// length = 66
	attron(COLOR_PAIR(1));
	mvprintw(1, (col - 66) / 2, "@@@@@@@   @@@@@@@    @@@@@@   @@@@@@@   @@@@@@   @@@@@@@   @@@@@@ ");
	mvprintw(2, (col - 66) / 2, "@@@@@@@@  @@@@@@@@  @@@@@@@   @@@@@@@  @@@@@@@@  @@@@@@@  @@@@@@@ ");
	mvprintw(3, (col - 66) / 2, "@@!  @@@  @@!  @@@  !@@         @@!    @@!  @@@    @@!    !@@     ");
	mvprintw(4, (col - 66) / 2, "!@!  @!@  !@!  @!@  !@!         !@!    !@!  @!@    !@!    !@!     ");
	mvprintw(5, (col - 66) / 2, "@!@  !@!  @!@  !@!  !!@@!!      @!!    @!@!@!@!    @!!    !!@@!!  ");
	mvprintw(6, (col - 66) / 2, "!@!  !!!  !@!  !!!   !!@!!!     !!!    !!!@!!!!    !!!     !!@!!! ");
	mvprintw(7, (col - 66) / 2, "!!:  !!!  !!:  !!!       !:!    !!:    !!:  !!!    !!:         !:!");
	mvprintw(8, (col - 66) / 2, ":!:  !:!  :!:  !:!      !:!     :!:    :!:  !:!    :!:        !:! ");
	mvprintw(9, (col - 66) / 2, " :::: ::   :::: ::  :::: ::      ::    ::   :::     ::    :::: :: ");
	mvprintw(10, (col - 66) / 2, ":: :  :   :: :  :   :: : :       :      :   : :     :     :: : : ");
	mvprintw(11, ((col - 66) / 2) + (66 - version.length()-2), "v%s", version.c_str());
	attroff(COLOR_PAIR(1));

}
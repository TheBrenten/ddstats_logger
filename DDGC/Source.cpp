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
std::string version = "0.2.2";
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
void printTitle();
void printStatus();
void printMonitorStats();
void printStats();
int getReplayPlayerID(HANDLE process);
bool replayUsernameMatch();
void debug(std::string debugStr);
void printDebug();

float inGameTimerSubmit = 0.0;
int gemsSubmit = 0;
int homingDaggersSubmit = 0;
int daggersFiredSubmit = 0;
int daggersHitSubmit = 0;
int enemiesAliveSubmit = 0;
int enemiesKilledSubmit = 0;
float accuracySubmit = 0.0;

int row, col;
int rown;
int leftAlign, rightAlign, centerAlign;

string gameName = "Devil Daggers";
LPCSTR gameWindow = "Devil Daggers";
string gameStatus;
uintptr_t exeBaseAddress = 0;

bool isGameAvail;
bool updateOnNextRun;
bool recording = false;
bool monitorStats = false;
bool replayEnabled = false;

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
DWORD playerPointer = 0x001F30C0;
DWORD gamePointer = 0x001F8084;

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
int replayPlayerID;

std::vector<std::string> debugVector;

 
int main() {

	// set up curses
	WINDOW *stdscr = initscr();
	raw();
	keypad(stdscr, TRUE);
	noecho();
	start_color();
	curs_set(0);

	getmaxyx(stdscr, row, col);

	rown = 12;
	leftAlign = (col - 66) / 2;
	rightAlign = leftAlign + 66;
	centerAlign = col / 2;

	// color setup
	init_pair(1, COLOR_RED, COLOR_BLACK);
	init_pair(2, COLOR_GREEN, COLOR_BLACK);
	init_pair(3, COLOR_MAGENTA, COLOR_BLACK);
	init_pair(4, COLOR_YELLOW, COLOR_BLACK);

	HWND hGameWindow = NULL;
	int timeSinceLastUpdate = clock();
	int gameAvailTimer = clock();
	int onePressTimer = clock();

	DWORD dwProcID = NULL;
	updateOnNextRun = true;

	future_motd_response = getMOTD();

	while (!GetAsyncKeyState(VK_F10)) {
		if ((clock() - gameAvailTimer > 100) && validVersion) {
			gameAvailTimer = clock();
			isGameAvail = false;

			hGameWindow = FindWindow(NULL, gameWindow);
			if (hGameWindow) {
				GetWindowThreadProcessId(hGameWindow, &dwProcID);
				if (dwProcID) {
					hProcHandle = OpenProcess(PROCESS_VM_READ | PROCESS_QUERY_INFORMATION, FALSE, dwProcID);
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
				rown = 12; // row number

				printTitle();

				printStatus();

				if (monitorStats || isReplay)
					printMonitorStats();
				else
					printStats();

				if (monitorStats) {
					attron(COLOR_PAIR(3));
					mvprintw(0, leftAlign, "+s");
					attroff(COLOR_PAIR(3));
				}
				if (replayEnabled) {
					attron(COLOR_PAIR(4));
					mvprintw(0, leftAlign + 2, "+r");
					attroff(COLOR_PAIR(4));
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
				rown++;
				mvprintw(rown, leftAlign, "Last Submission:");
				if (errorLine != "") {
					attron(COLOR_PAIR(1));
					mvprintw(rown, leftAlign + 17, errorLine.c_str());
					attroff(COLOR_PAIR(1));
				} else if (!jsonResponse.empty()) {
					attron(COLOR_PAIR(2));
					mvprintw(rown, leftAlign + 17, "https://ddstats.com/game_log/%s", to_string(jsonResponse.at("game_id").get<std::int32_t>()).c_str());
					attroff(COLOR_PAIR(2));
				} else {
					mvprintw(rown, leftAlign + 17, "None, yet.");
				}
				rown++;
				mvprintw(rown, leftAlign, "[F10] Exit");


				#if defined _DEBUG
					printDebug();
				#endif

				refresh();

				updateOnNextRun = false;
				timeSinceLastUpdate = clock();
			}

			if (isGameAvail) {
				//writeToMemory(hProcHandle);
				collectGameVars(hProcHandle);
				if (alive && inGameTimer > 0) {
					if ((!isReplay) || (isReplay && replayEnabled)) {
						recording = true;
						if (recordingCounter < inGameTimer) {
							commitVectors();
						}
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
					if (GetAsyncKeyState(VK_F12)) {
						onePressTimer = clock();
						replayEnabled = !replayEnabled;
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

		// this is to make sure all points are captured when fast-forwarding
		if (isReplay && replayEnabled)
			Sleep(5);
		else
			Sleep(15); // cut back on some(?) overhead
	}

	endwin();
	CloseHandle(hProcHandle);
	return ERROR_SUCCESS;
}

void debug(std::string debugStr) {
	debugVector.push_back(debugStr);
}

void printDebug() {
	attron(COLOR_PAIR(1));
	rown += 2;
	if (!debugVector.empty())
		mvprintw(rown, leftAlign, "*************************DEBUG***********************");
	attron(A_BOLD);
	for (std::string d : debugVector) {
		rown++;
		mvprintw(rown, leftAlign, d.c_str());
	}
	attroff(COLOR_PAIR(1));
	attroff(A_BOLD);
}

void printStatus() {
	if (future_motd_response.valid()) {
		if (future_motd_response.wait_for(std::chrono::seconds(0)) == std::future_status::ready) {
			auto r = future_motd_response.get();
			if (r.status_code >= 400 || r.status_code == 0) {
				motd = "Error [" + to_string(r.status_code) + "] connecting to ddstats.com.";
			}
			else {
				motd = json::parse(r.text).at("motd").get<std::string>();
				validVersion = json::parse(r.text).at("valid_version").get<bool>();
				updateAvailable = json::parse(r.text).at("update_available").get<bool>();
			}
		}
	}
	if (updateAvailable) {
		attron(COLOR_PAIR(2));
		mvprintw(rown - 1, leftAlign, "(UPDATE AVAILABLE)");
		attroff(COLOR_PAIR(2));
	}
	mvprintw(rown, (col - motd.length()) / 2, "%s", motd.c_str());
	rown++;
	if (isGameAvail)
		attron(COLOR_PAIR(2));
	else
		attron(COLOR_PAIR(1));
	mvprintw(rown, (col - gameStatus.length()) / 2, gameStatus.c_str());
	if (isGameAvail)
		attroff(COLOR_PAIR(2));
	else
		attroff(COLOR_PAIR(1));
	rown++;
	if (recording && (inGameTimer != 0.0)) {
		attron(COLOR_PAIR(2));
		attron(A_BOLD);
		if (isReplay && replayEnabled)
			recordingStatus = " [[ Recording Replay ]] ";
		else
			recordingStatus = " [[ Recording ]] ";
	}
	else {
		attron(COLOR_PAIR(1));
		recordingStatus = "[[ Not Recording ]]";
	}
	mvprintw(rown, (col - recordingStatus.length()) / 2, recordingStatus.c_str());
	if (recording && (inGameTimer != 0.0)) {
		attroff(COLOR_PAIR(2));
		attroff(A_BOLD);
	}
	else
		attroff(COLOR_PAIR(1));
	rown++;
}

void printMonitorStats() {
	// left column
	mvprintw(rown, leftAlign, "In Game Timer: %.4fs", inGameTimer);
	rown++;
	mvprintw(rown, leftAlign, "Daggers Hit: %d", daggersHit);
	rown++;
	mvprintw(rown, leftAlign, "Daggers Fired: %d", daggersFired);
	rown++;
	if (daggersFired > 0.0)
		mvprintw(rown, leftAlign, "Accuracy: %.2f%%", ((float)daggersHit / (float)daggersFired) * 100.0);
	else
		mvprintw(rown, leftAlign, "Accuracy: 0.00%%", daggersFired);

	// right column
	rown -= 3;
	mvprintw(rown, rightAlign - (6 + to_string(gems).length()), "Gems: %d", gems);
	rown++;
	mvprintw(rown, rightAlign - (16 + to_string(homingDaggers).length()), "Homing Daggers: %d", homingDaggers);
	rown++;
	// fix for title screen
	if (inGameTimer == 0.0)
		mvprintw(rown, rightAlign - 16, "Enemies Alive: %d", 0);
	else
		mvprintw(rown, rightAlign - (15 + to_string(enemiesAlive).length()), "Enemies Alive: %d", enemiesAlive);
	rown++;
	if (inGameTimer = 0.0)
		mvprintw(rown, rightAlign - 17, "Enemies Killed: %d", 0);
	else
		mvprintw(rown, rightAlign - (16 + to_string(enemiesKilled).length()), "Enemies Killed: %d", enemiesKilled);
	rown++;
}

void printStats() {

	// left column
	mvprintw(rown, leftAlign, "In Game Timer: %.4fs", inGameTimerSubmit);
	rown++;
	mvprintw(rown, leftAlign, "Daggers Hit: %d", daggersHitSubmit);
	rown++;
	mvprintw(rown, leftAlign, "Daggers Fired: %d", daggersFiredSubmit);
	rown++;
	mvprintw(rown, leftAlign, "Accuracy: %.2f%%", accuracySubmit);

	// right column
	rown -= 3;
	mvprintw(rown, rightAlign - (6 + to_string(gemsSubmit).length()), "Gems: %d", gemsSubmit);
	rown++;
	mvprintw(rown, rightAlign - (16 + to_string(homingDaggersSubmit).length()), "Homing Daggers: %d", homingDaggersSubmit);
	rown++;
	mvprintw(rown, rightAlign - (15 + to_string(enemiesAliveSubmit).length()), "Enemies Alive: %d", enemiesAliveSubmit);
	rown++;
	mvprintw(rown, rightAlign - (16 + to_string(enemiesKilledSubmit).length()), "Enemies Killed: %d", enemiesKilledSubmit);
	rown++;

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
	DWORD playerBaseAddress;
	DWORD gameBaseAddress;
	DWORD pointerAddr;

	pointer = exeBaseAddress + playerPointer;
	if (!ReadProcessMemory(hProcHandle, (LPCVOID)pointer, &playerBaseAddress, sizeof(playerBaseAddress), NULL)) {
		cout << " Failed to read address for player base address." << endl;
		return;
	}

	pointer = exeBaseAddress + gamePointer;
	if (!ReadProcessMemory(hProcHandle, (LPCVOID)pointer, &gameBaseAddress, sizeof(gameBaseAddress), NULL)) {
		cout << " Failed to read address for game base address." << endl;
		return;
	}

	// inGameTimer
	pointer = playerBaseAddress + inGameTimerOffset;
	oldInGameTimer = inGameTimer;
	ReadProcessMemory(hProcHandle, (LPCVOID)pointer, &inGameTimer, sizeof(inGameTimer), NULL);
	if ((inGameTimer < oldInGameTimer) && recording && (oldInGameTimer > 4.0)) {
		if (!isReplay) {
			// submit, but use oldInGameTimer var instead of new... then continue.
			deathType = -1; // did not die, so no death type.
			future_response = sendToServer();
		}
		resetVectors(); // reset after submit or if player restarted
	}

	// isReplay
	pointer = playerBaseAddress + isReplayOffset;
	ReadProcessMemory(hProcHandle, (LPCVOID)pointer, &isReplay, sizeof(isReplay), NULL);

	// alive
	pointer = playerBaseAddress + aliveOffset;
	ReadProcessMemory(hProcHandle, (LPCVOID)pointer, &alive, sizeof(alive), NULL);

	// gems
	pointer = playerBaseAddress + gemsOffset;
	ReadProcessMemory(hProcHandle, (LPCVOID)pointer, &gems, sizeof(gems), NULL);

	// homingDaggers
	//pointer = gameBaseAddress + homingDaggersOffset;
	// 2 pointer offsets for homingDaggers
	pointerAddr = gameBaseAddress + 0x0;
	ReadProcessMemory(hProcHandle, (LPCVOID)pointerAddr, &pointer, sizeof(pointer), NULL);
	pointerAddr = pointer + homingDaggersOffset;
	ReadProcessMemory(hProcHandle, (LPCVOID)pointerAddr, &homingDaggers, sizeof(homingDaggers), NULL);

	// daggersFired
	pointer = playerBaseAddress + daggersFiredOffset;
	ReadProcessMemory(hProcHandle, (LPCVOID)pointer, &daggersFired, sizeof(daggersFired), NULL);

	// daggersHit
	pointer = playerBaseAddress + daggersHitOffset;
	ReadProcessMemory(hProcHandle, (LPCVOID)pointer, &daggersHit, sizeof(daggersHit), NULL);

	// enemiesKilled
	pointer = playerBaseAddress + enemiesKilledOffset;
	ReadProcessMemory(hProcHandle, (LPCVOID)pointer, &enemiesKilled, sizeof(enemiesKilled), NULL);
	
	// enemiesAlive
	pointer = playerBaseAddress + enemiesAliveOffset;
	ReadProcessMemory(hProcHandle, (LPCVOID)pointer, &enemiesAlive, sizeof(enemiesAlive), NULL);
	
	// deathType
	pointer = playerBaseAddress + deathTypeOffset;
	ReadProcessMemory(hProcHandle, (LPCVOID)pointer, &deathType, sizeof(deathType), NULL);
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

	// this fixes the last captured time if user restarts to be accurate
	float inGameTimerFix;
	if (deathType == -1) {
		inGameTimerFix = oldInGameTimer;
	} else {
		inGameTimerFix = inGameTimer;
	}

	if (isReplay && !alive) {
		if (replayUsernameMatch())
			replayPlayerID = playerID;
		else
			replayPlayerID = getReplayPlayerID(hProcHandle);
	} else {
		replayPlayerID = 0;
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
		{ "enemiesAlive", enemiesAlive },
		{ "enemiesAliveVector", enemiesAliveVector },
		{ "enemiesKilled", enemiesKilled },
		{ "enemiesKilledVector", enemiesKilledVector },
		{ "deathType", deathType },
		{ "replayPlayerID", replayPlayerID }
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
		accuracySubmit = ( daggersHit / daggersFired ) * 100.0;

	auto future_response = cpr::PostCallback([](cpr::Response r) {
		return r;
	}, 
		cpr::Url{ "http://ddstats.com/api/submit_game" },
		cpr::Body{ log.dump() }, 
		cpr::Header{ { "Content-Type", "application/json" } });

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

void printTitle() {

	// length = 66
	attron(COLOR_PAIR(1));
	mvprintw(1, (col - 66) / 2,  "@@@@@@@   @@@@@@@    @@@@@@   @@@@@@@   @@@@@@   @@@@@@@   @@@@@@ ");
	mvprintw(2, (col - 66) / 2,  "@@@@@@@@  @@@@@@@@  @@@@@@@   @@@@@@@  @@@@@@@@  @@@@@@@  @@@@@@@ ");
	mvprintw(3, (col - 66) / 2,  "@@!  @@@  @@!  @@@  !@@         @@!    @@!  @@@    @@!    !@@     ");
	mvprintw(4, (col - 66) / 2,  "!@!  @!@  !@!  @!@  !@!         !@!    !@!  @!@    !@!    !@!     ");
	mvprintw(5, (col - 66) / 2,  "@!@  !@!  @!@  !@!  !!@@!!      @!!    @!@!@!@!    @!!    !!@@!!  ");
	mvprintw(6, (col - 66) / 2,  "!@!  !!!  !@!  !!!   !!@!!!     !!!    !!!@!!!!    !!!     !!@!!! ");
	mvprintw(7, (col - 66) / 2,  "!!:  !!!  !!:  !!!       !:!    !!:    !!:  !!!    !!:         !:!");
	mvprintw(8, (col - 66) / 2,  ":!:  !:!  :!:  !:!      !:!     :!:    :!:  !:!    :!:        !:! ");
	mvprintw(9, (col - 66) / 2,  " :::: ::   :::: ::  :::: ::      ::    ::   :::     ::    :::: :: ");
	mvprintw(10, (col - 66) / 2, ":: :  :   :: :  :   :: : :       :      :   : :     :     :: : : ");
	mvprintw(11, ((col - 66) / 2) + (66 - version.length()-2), "v%s", version.c_str());
	attroff(COLOR_PAIR(1));

}

// this function takes forever in debug mode... don't know why..
int getReplayPlayerID(HANDLE process)
{
	if (process)
	{
		const char data[] = { '"', 'r', 'e', 'p', 'l', 'a', 'y', '"' };
		size_t len = sizeof(data);

		SYSTEM_INFO si;
		GetSystemInfo(&si);

		MEMORY_BASIC_INFORMATION info;
		std::vector<char> chunk;
		char* p = 0;
		while (p < si.lpMaximumApplicationAddress)
		{
			if (VirtualQueryEx(process, p, &info, sizeof(info)) == sizeof(info))
			{
				p = (char*)info.BaseAddress;
				chunk.resize(info.RegionSize);
				SIZE_T bytesRead;
				if (ReadProcessMemory(process, p, &chunk[0], info.RegionSize, &bytesRead))
				{
					for (size_t i = 0; i < (bytesRead - len); ++i)
					{
						if (memcmp(data, &chunk[i], len) == 0)
						{
							int id;
							char value[7] = { 0 };
							ReadProcessMemory(process, (LPVOID)(p + i + 12), &value, sizeof(value), 0);
							value[6] = '\0';
							sscanf_s(value, "%d", &id);
							return id;
						}
					}
				}
				p += info.RegionSize;
			}
		}
	}
	return 0;
}

bool replayUsernameMatch() {
	DWORD pointer;
	DWORD pTemp;
	DWORD pointerAddr;

	// player name char count
	pointer = exeBaseAddress + 0x001F30C0;
	if (!ReadProcessMemory(hProcHandle, (LPCVOID)pointer, &pTemp, sizeof(pTemp), NULL)) {
		cout << " Failed to read address for player base." << endl;
	}
	else {
		int playerNameCharCount;
		int replayNameCharCount;

		// player name char count
		pointerAddr = pTemp + 0x70;
		ReadProcessMemory(hProcHandle, (LPCVOID)pointerAddr, &playerNameCharCount, sizeof(playerNameCharCount), NULL);
		// replay name char count
		pointerAddr = pTemp + 0x370;
		ReadProcessMemory(hProcHandle, (LPCVOID)pointerAddr, &replayNameCharCount, sizeof(replayNameCharCount), NULL);
		if (playerNameCharCount != replayNameCharCount)
			return false;

		char playerName[128];
		char replayName[128];

		// player name
		pointerAddr = pTemp + 0x60;
		ReadProcessMemory(hProcHandle, (LPCVOID)pointerAddr, &playerName, sizeof(playerName), NULL);
		// replay name
		pointerAddr = pTemp + 0x360;
		ReadProcessMemory(hProcHandle, (LPCVOID)pointerAddr, &replayName, sizeof(replayName), NULL);
		
		if (!strncmp(playerName, replayName, playerNameCharCount)) {
			return true;
		}
	}

	return false;
}
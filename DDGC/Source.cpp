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
#define NCURSES_MOUSE_VERSION 2
#include <curses.h>
#include <Psapi.h>
#include "md5.h"
#include "sio_client.h"

using namespace std;
using json = nlohmann::json;

// current version
std::string version = "0.3.2";
std::string title = "DDSTATS v" + version;
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
std::string getSurvivalHash();
bool copyToClipboard(string source);

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
// int ch;

string gameName = "Devil Daggers";
LPCSTR gameWindow = "Devil Daggers";
string gameStatus;
uintptr_t exeBaseAddress = 0;

bool isGameAvail;
bool updateOnNextRun;
bool recording = false;
bool monitorStats = false;
bool live = false;

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

// socketio VARS
std::string sioStatus = "[[ Offline ]]";
std::string playerIDNamespace;
bool streaming = false;
bool onlineToggle = true;
double lastSioInGameTimerSubmit;
int lastSioStatusSubmit = -2;
int user_count = 0;

// GAME VARS
DWORD playerPointer = 0x001F30C0;
DWORD gamePointer = 0x001F8084;

float oldInGameTimer;
float inGameTimer;
DWORD inGameTimerBaseAddress = 0x001F30C0;
DWORD inGameTimerOffset = 0x1A0;
vector <float> inGameTimerVector;
int playerID = -1;
DWORD playerIDBaseAddress = 0x001F30C0;
DWORD playerIDOffset = 0x5C;
int gemsUpgrade;
DWORD gemsUpgradeOffset = 0x218;
double levelTwo = 0.0;
double levelThree = 0.0;
double levelFour = 0.0;
double levelTwoSubmit = 0.0;
double levelThreeSubmit = 0.0;
double levelFourSubmit = 0.0;
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
int enemiesAliveMax = 0;
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
DWORD playerNameCharCountOffset = 0x70;
int playerNameCharCount;
DWORD playerNameOffset = 0x60;
char playerName[128];
DWORD replayPlayerNameCharCountOffset = 0x370;
int replayPlayerNameCharCount;
DWORD replayPlayerNameOffset = 0x360;
char replayPlayerName[128];
int homingDaggersMaxTotal = 0;
double homingDaggersMaxTotalTime = 0.0;
int enemiesAliveMaxTotal = 0;
double enemiesAliveMaxTotalTime = 0.0;

std::vector<std::string> debugVector;

 
int main() {
	SetConsoleTitle(title.c_str());

	sio::client h;

	h.connect("http://www.ddstats.com");
	// h.connect("http://10.0.1.222:5666");

	// set up curses
	// MEVENT event;
	WINDOW *stdscr = initscr();
	raw();
	// nodelay(stdscr, true);
	keypad(stdscr, true);
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
	mousemask(ALL_MOUSE_EVENTS, NULL);
	// int ch;

	HWND hGameWindow = NULL;
	int timeSinceLastSubmission = clock();
	int timeSinceLastUpdate = clock();
	int gameAvailTimer = clock();
	int onePressTimer = clock();
	int copyClickTimer = clock();

	DWORD dwProcID = NULL;
	updateOnNextRun = true;

	future_motd_response = getMOTD();

	while (!GetAsyncKeyState(VK_F10)) {

		if (live && !h.opened()) live = false;

		if (!live && isGameAvail && (playerID != -1) && h.opened() && onlineToggle) {
			// std::string test_msg = "testing";
			h.socket()->emit("login", to_string(playerID));
			live = true;
		}

		if (live && alive && (inGameTimer > 0.0) && onlineToggle) streaming = true;
		else streaming = false;

		//if (live && onlineToggle) {
		//	h.socket("/stats")->on("update_user_count", sio::socket::event_listener_aux([&](string const& name, sio::message::ptr const& data, bool isAck, sio::message::list &ack_resp) {
		//		// user_count = ev.get_message()->get_int();
		//		user_count++;
		//	}));
		//}

		if (live && !streaming && onlineToggle) {
			sioStatus = "[[ Online ]]";
		} else if (streaming) sioStatus = "[[ Streaming Stats ]]";
		else if (!live && onlineToggle) sioStatus = "[[ Connecting... ]]";
		else sioStatus = "[[ Offline ]]";

		if (!lastGameSubmission.empty() && GetAsyncKeyState(VK_LBUTTON) && (GetConsoleWindow() == GetForegroundWindow())) {
			copyToClipboard(lastGameSubmission);
			copyClickTimer = clock();
		}

		if ((clock() - gameAvailTimer > 100) && validVersion) {
			gameAvailTimer = clock();
			isGameAvail = false;

			hGameWindow = FindWindow(NULL, gameWindow);
			if (hGameWindow) {
				GetWindowThreadProcessId(hGameWindow, &dwProcID);
				if (dwProcID) {
					hProcHandle = OpenProcess(PROCESS_VM_READ | PROCESS_QUERY_INFORMATION, FALSE, dwProcID);
					if (hProcHandle == INVALID_HANDLE_VALUE || hProcHandle == NULL) {
						gameStatus = "Failed to open process handle. Try running as Administrator.";
					} else {
						gameStatus = "[[ Connected to Devil Daggers ]]";
						exeBaseAddress = GetModuleBaseAddress(dwProcID, (_TCHAR*)_T("dd.exe"));
						isGameAvail = true;
					}
				} else {
					gameStatus = "Failed to get process ID. Try running as Administrator.";
				}
			} else {
				gameStatus = "[[ Devil Daggers not found ]]";
				// this fixes a bug when people keep ddstats open, close dd, and restart dd (hopefully)
				enemiesAlive = 0;
			}

			// SOCKETIO STUFF
			// if the connection is made, and everything is connected and good.. submit stats...
			if (clock() - timeSinceLastSubmission > 500) {
				if (streaming || ((lastSioInGameTimerSubmit != inGameTimerSubmit) && !alive) || (!alive && lastSioStatusSubmit != deathType) ||
					(alive && inGameTimer == 0.0 && lastSioStatusSubmit != -2)) {
					lastSioInGameTimerSubmit = inGameTimer;
					sio::message::list sioStatList;
					sioStatList.push(sio::int_message::create(playerID));
					sioStatList.push(sio::double_message::create(inGameTimer));
					sioStatList.push(sio::int_message::create(gems));
					sioStatList.push(sio::int_message::create(homingDaggers));
					sioStatList.push(sio::int_message::create(enemiesAlive));
					sioStatList.push(sio::int_message::create(enemiesKilled));
					sioStatList.push(sio::int_message::create(daggersHit));
					sioStatList.push(sio::int_message::create(daggersFired));
					sioStatList.push(sio::double_message::create(levelTwo));
					sioStatList.push(sio::double_message::create(levelThree));
					sioStatList.push(sio::double_message::create(levelFour));
					sioStatList.push(sio::bool_message::create(isReplay));
					if (!alive) lastSioStatusSubmit = deathType;
					else if (alive && inGameTimer == 0.0) lastSioStatusSubmit = -2; // in menu
					else lastSioStatusSubmit = -1; // alive
					sioStatList.push(sio::int_message::create(lastSioStatusSubmit));
					h.socket("/stats")->emit("submit", sioStatList);
					timeSinceLastSubmission = clock();
				}
			}

			//h.socket("/stats")->on("get_status", [&](sio::event& ev) {
			//	h.socket("/stats")->emit("status", sio::int_message::create(lastSioStatusSubmit));
			//});

			if (updateOnNextRun || clock() - timeSinceLastUpdate > 500) {

				clear();
				rown = 12; // row number

				if (monitorStats) {
					attron(COLOR_PAIR(3));
					mvprintw(0, leftAlign, "+s");
					attroff(COLOR_PAIR(3));
				}

				if (strlen(playerName) != 0) {
					attron(COLOR_PAIR(1));
					mvprintw(rown-1, leftAlign, playerName);
					attroff(COLOR_PAIR(1));
				}

				if (!isGameAvail && !live) {}
				else if (live && onlineToggle) { // Online
					attron(COLOR_PAIR(2));
					mvprintw(rown-1, (col - sioStatus.length()) / 2, sioStatus.c_str());
					attroff(COLOR_PAIR(2));
				}
				else if (!live && onlineToggle) { // Connecting
					attron(COLOR_PAIR(4));
					mvprintw(rown - 1, (col - sioStatus.length()) / 2, sioStatus.c_str());
					attroff(COLOR_PAIR(4));
				}
				else {
					attron(COLOR_PAIR(1)); // Offline
					mvprintw(rown-1, (col - sioStatus.length()) / 2, sioStatus.c_str());
					attroff(COLOR_PAIR(1));
				}

				printTitle();

				printStatus();

				if (monitorStats || isReplay)
					printMonitorStats();
				else
					printStats();

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
							h.socket("/stats")->emit("game_submitted", sio::int_message::create((int)jsonResponse.at("game_id").get<std::int32_t>()));
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
					std::ostringstream oss;
					oss <<  "https://ddstats.com/game_log/" << to_string(jsonResponse.at("game_id").get<std::int32_t>());
					lastGameSubmission = oss.str();
					attron(COLOR_PAIR(2));
					if (clock() - copyClickTimer < 1500)
						mvprintw(rown, leftAlign + 17, "(copied to clipboard)");
					else
						mvprintw(rown, leftAlign + 17, lastGameSubmission.c_str());
					attroff(COLOR_PAIR(2));
				} else {
					mvprintw(rown, leftAlign + 17, "None, yet.");
				}
				rown++;
				mvprintw(rown, leftAlign, "[F10] Exit");

				if (updateAvailable) {
					attron(COLOR_PAIR(2));
					mvprintw(rown, rightAlign - 18, "(UPDATE AVAILABLE)");
					attroff(COLOR_PAIR(2));
				}

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
					//if (inGameTimer < 1.0) {
					//	enemiesAliveMaxTotal = 0.0;
					//	homingDaggersMaxTotal = 0.0;
					//}
					recording = true;
					if (recordingCounter < inGameTimer) {
						commitVectors();
					}
				}
				if (!alive && recording == true) {
					recording = false;
					if (inGameTimer > 4.0) {
						// this is to assure the correct last entry when user dies
						if (!homingDaggersVector.empty())
							homingDaggers = homingDaggersVector.back();
						// submit
						commitVectors(); // one last commit to make sure death time is accurate
						future_response = sendToServer();
					}
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

			if (clock() - onePressTimer > 400) {
				if (GetAsyncKeyState(VK_F12)) {
					onePressTimer = clock();
					onlineToggle = !onlineToggle;
					if (!onlineToggle) h.close();
					else h.connect("http://www.ddstats.com");
					updateOnNextRun = true;
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
		if (isReplay)
			Sleep(1);
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
		if (isReplay)
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
	//mvprintw(rown, rightAlign - (14 + to_string(levelTwoSubmit).length()), "Level Two At: %.4fs", levelTwoSubmit);
	//rown++;

}

void commitVectors() {
	recordingCounter += interval;
	inGameTimerVector.push_back(inGameTimer);
	gemsVector.push_back(gems);
	homingDaggersVector.push_back(homingDaggers);
	daggersFiredVector.push_back(daggersFired);
	daggersHitVector.push_back(daggersHit);
	if (inGameTimer < 1.0) enemiesAliveMax = 0; // fixes issue with enemies alive being recorded at title screen
	enemiesAliveVector.push_back(enemiesAliveMax);
	enemiesAliveMax = 0;
	enemiesKilledVector.push_back(enemiesKilled);
	if (gemsUpgrade >= 10 && gemsUpgrade < 70 && levelTwo == 0.0) levelTwo = inGameTimer;
	if (gemsUpgrade == 70 && levelThree == 0.0) levelThree = inGameTimer;
	if (gemsUpgrade == 71 && levelFour == 0.0) levelFour = inGameTimer;
}

void resetVectors() {
	recordingCounter = 0;
	inGameTimerVector.clear();
	gemsVector.clear();
	homingDaggersVector.clear();
	daggersFiredVector.clear();
	daggersHitVector.clear();
	enemiesAliveVector.clear();
	enemiesAliveMax = 0;
	enemiesKilledVector.clear();
	// clear out the replay player's name
	std::fill(std::begin(replayPlayerName), std::end(replayPlayerName), '\0');
	levelTwo = 0.0;
	levelThree = 0.0;
	levelFour = 0.0;
	homingDaggersMaxTotal = 0;
	homingDaggersMaxTotalTime = 0.0;
	enemiesAliveMaxTotal = 0;
	enemiesAliveMaxTotalTime = 0.0;
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

	// gems upgrade
	pointerAddr = gameBaseAddress + 0x0;
	ReadProcessMemory(hProcHandle, (LPCVOID)pointerAddr, &pointer, sizeof(pointer), NULL);
	pointerAddr = pointer + gemsUpgradeOffset;
	ReadProcessMemory(hProcHandle, (LPCVOID)pointerAddr, &gemsUpgrade, sizeof(gemsUpgrade), NULL);

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
	if (homingDaggers < 0) homingDaggers = 0; // fixes -1 bug
	if ((homingDaggers > homingDaggersMaxTotal) && recording) {
		homingDaggersMaxTotal = homingDaggers;
		homingDaggersMaxTotalTime = inGameTimer;
	}

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
	if (enemiesAlive > enemiesAliveMax) // since variables reset, this chooses the max variable over the second
		enemiesAliveMax = enemiesAlive;
	if ((enemiesAlive > enemiesAliveMaxTotal) && recording) {
		enemiesAliveMaxTotal = enemiesAlive;
		enemiesAliveMaxTotalTime = inGameTimer;
	}
	
	// deathType
	pointer = playerBaseAddress + deathTypeOffset;
	ReadProcessMemory(hProcHandle, (LPCVOID)pointer, &deathType, sizeof(deathType), NULL);

	// get playerID while recording iff it hasn't been retrieved yet
	if (playerID == -1) {// && recording) {
		pointer = playerBaseAddress + playerIDOffset;
		ReadProcessMemory(hProcHandle, (LPCVOID)pointer, &playerID, sizeof(playerID), NULL);
	}

	// get user name
	if (strlen(playerName) == 0 && recording) {
		pointer = playerBaseAddress + playerNameCharCountOffset;
		ReadProcessMemory(hProcHandle, (LPCVOID)pointer, &playerNameCharCount, sizeof(playerNameCharCount), NULL);

		pointer = playerBaseAddress + playerNameOffset;
		ReadProcessMemory(hProcHandle, (LPCVOID)pointer, &playerName, sizeof(playerName), NULL);

		playerName[playerNameCharCount] = '\0';
	}

	// get replay user name
	if (strlen(replayPlayerName) == 0 && recording && isReplay) {
		pointer = playerBaseAddress + replayPlayerNameCharCountOffset;
		ReadProcessMemory(hProcHandle, (LPCVOID)pointer, &replayPlayerNameCharCount, sizeof(replayPlayerNameCharCount), NULL);

		pointer = playerBaseAddress + replayPlayerNameOffset;
		ReadProcessMemory(hProcHandle, (LPCVOID)pointer, &replayPlayerName, sizeof(replayPlayerName), NULL);

		replayPlayerName[replayPlayerNameCharCount] = '\0';
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
	 
	// get playerID if it hasn't been retreieved yet..
	if (playerID == -1) {
		pointer = exeBaseAddress + playerIDBaseAddress;
		if (!ReadProcessMemory(hProcHandle, (LPCVOID)pointer, &pTemp, sizeof(pTemp), NULL)) {
			cout << " Failed to read address for playerID." << endl;
		}
		else {
			pointerAddr = pTemp + playerIDOffset;
			ReadProcessMemory(hProcHandle, (LPCVOID)pointerAddr, &playerID, sizeof(playerID), NULL);
		}
	}

	// this fixes the last captured time if user restarts to be accurate
	float inGameTimerFix;
	if (deathType == -1) {
		inGameTimerFix = oldInGameTimer;
	} else {
		inGameTimerFix = inGameTimer;
	}

	if (isReplay && !alive) {
		if (replayUsernameMatch()) {
			replayPlayerID = playerID;
			strcpy_s(replayPlayerName, playerName);
		} else {
			replayPlayerID = getReplayPlayerID(hProcHandle);
		}
	} else {
		replayPlayerID = 0;
		strcpy_s(replayPlayerName, "none");
	}

	std::string survivalHash = getSurvivalHash();

	json log = {
		{ "playerID", playerID },
		{ "playerName", playerName },
		{ "granularity", interval },
		{ "inGameTimer", inGameTimerFix },
		{ "inGameTimerVector", inGameTimerVector },
		{ "gems", gems },
		{ "gemsVector", gemsVector },
		{ "levelTwoTime", levelTwo },
		{ "levelThreeTime", levelThree },
		{ "levelFourTime", levelFour },
		{ "homingDaggers", homingDaggers },
		{ "homingDaggersVector", homingDaggersVector },
		{ "homingDaggersMax", homingDaggersMaxTotal },
		{ "homingDaggersMaxTime", homingDaggersMaxTotalTime },
		{ "daggersFired", daggersFired },
		{ "daggersFiredVector", daggersFiredVector },
		{ "daggersHit", daggersHit },
		{ "daggersHitVector", daggersHitVector },
		{ "enemiesAlive", enemiesAlive },
		{ "enemiesAliveVector", enemiesAliveVector },
		{ "enemiesAliveMax", enemiesAliveMaxTotal },
		{ "enemiesAliveMaxTime", enemiesAliveMaxTotalTime },
		{ "enemiesKilled", enemiesKilled },
		{ "enemiesKilledVector", enemiesKilledVector },
		{ "deathType", deathType },
		{ "replayPlayerID", replayPlayerID },
		{ "version", version },
		{ "survivalHash", survivalHash }
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
		accuracySubmit = (float) (( daggersHit / daggersFired ) * 100.0);
	levelTwoSubmit = levelTwo;
	levelThreeSubmit = levelThree;
	levelFourSubmit = levelFour;

	auto future_response = cpr::PostCallback([](cpr::Response r) {
		return r;
	}, 
		// cpr::Url{ "http://10.0.1.222:5666/api/submit_game" },
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
	pointer = exeBaseAddress + playerPointer;
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

std::string getSurvivalHash() {
	wchar_t buf[MAX_PATH];
	if (hProcHandle) GetModuleFileNameExW(hProcHandle, 0, buf, MAX_PATH);
	wstring ws(buf);
	string str(ws.begin(), ws.end());
	str = str.substr(0, str.size() - 4);
	str += "\\survival";
	std::ifstream fp(str);
	std::stringstream ss;
	ss << fp.rdbuf();
	return md5(ss.str());
}

bool copyToClipboard(string source) {
	if (OpenClipboard(NULL))
	{
		HGLOBAL clipbuffer;
		char * buffer;
		EmptyClipboard();
		clipbuffer = GlobalAlloc(GMEM_DDESHARE, source.size() + 1);
		buffer = (char*)GlobalLock(clipbuffer);
		strcpy(buffer, LPCSTR(source.c_str()));
		GlobalUnlock(clipbuffer);
		SetClipboardData(CF_TEXT, clipbuffer);
		CloseClipboard();
		return true;
	}
	return false;
}
#pragma once
// these boys have their own file so compiling isnt quite a bitch

using namespace std;
using json = nlohmann::json;

// these functions are not needed anymore yay
//int getReplayPlayerID(HANDLE process);
//bool replayUsernameMatch();
//void printMonitorStats();

void printHelpMenu() {
    say("F10 or Alt+F4: Close DDStats.");
    say("          F12: Toggle live streaming your stats.");
    say("    Shift+F12: Download a weekly challenge spawnset.");

}
#include <ostream>
void replaceSpawnset(unsigned char* spawnset, int size) { // unsigned char*?
    std::string filePath = "C:/Program Files (x86)/Steam/steamapps/common/devildaggers/dd/survival";
    std::ofstream output(filePath, std::ios::out | std::ios::binary);
    output.write(reinterpret_cast<char*>(spawnset), size);
    say("size: " << size);
    Sleep(5000);
}



//void printStats() {
//
//	// left column
//	mvprintw(rown, leftAlign, "In Game Timer: %.4fs", inGameTimerSubmit);
//	rown++;
//	mvprintw(rown, leftAlign, "Daggers Hit: %d", daggersHitSubmit);
//	rown++;
//	mvprintw(rown, leftAlign, "Daggers Fired: %d", daggersFiredSubmit);
//	rown++;
//	mvprintw(rown, leftAlign, "Accuracy: %.2f%%", accuracySubmit);
//
//	// right column
//	rown -= 3;
//	mvprintw(rown, rightAlign - (6 + to_string(gemsSubmit).length()), "Gems: %d", gemsSubmit);
//	rown++;
//	mvprintw(rown, rightAlign - (16 + to_string(homingDaggersSubmit).length()), "Homing Daggers: %d", homingDaggersSubmit);
//	rown++;
//	mvprintw(rown, rightAlign - (15 + to_string(enemiesAliveSubmit).length()), "Enemies Alive: %d", enemiesAliveSubmit);
//	rown++;
//	mvprintw(rown, rightAlign - (16 + to_string(enemiesKilledSubmit).length()), "Enemies Killed: %d", enemiesKilledSubmit);
//	rown++;
//	//mvprintw(rown, rightAlign - (14 + to_string(levelTwoSubmit).length()), "Level Two At: %.4fs", levelTwoSubmit);
//	//rown++;
//
//}
//

int recordingCounter = 0;
int interval = 1; // seconds per update
int enemiesAliveMax = 0;
vector<float> inGameTimerVector;
vector <int> gemsVector;
vector <int> homingDaggersVector;
vector <int> daggersFiredVector;
vector <int> daggersHitVector;
vector <int> enemiesAliveVector;
vector <int> enemiesKilledVector;
float lvl2Time = 0.0;
float lvl3Time = 0.0;
float lvl4Time = 0.0;
int homingDaggersMaxTotal = 0;
double homingDaggersMaxTotalTime = 0.0;
int enemiesAliveMaxTotal = 0;
double enemiesAliveMaxTotalTime = 0.0;


void commitVectors() {
    recordingCounter += interval;
    inGameTimerVector.push_back(timer);
    gemsVector.push_back(totalGems);
    homingDaggersVector.push_back(homing);
    daggersFiredVector.push_back(daggersFired);
    daggersHitVector.push_back(daggersHit);
    if (timer < 1.0) enemiesAliveMax = 0; // fixes issue with enemies alive being recorded at title screen
    enemiesAliveVector.push_back(enemiesAliveMax);
    enemiesAliveMax = 0;
    enemiesKilledVector.push_back(enemiesKilled);
    if (totalGems >= 10 && totalGems < 70 
                        && lvl2Time == 0.0) lvl2Time = timer;
    if (totalGems == 70 && lvl3Time == 0.0) lvl3Time = timer;
    if (totalGems == 71 && lvl4Time == 0.0) lvl4Time = timer;
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
    std::fill(std::begin(replayName), std::end(replayName), '\0');
    lvl2Time = 0.0;
    lvl3Time = 0.0;
    lvl4Time = 0.0;
    homingDaggersMaxTotal = 0;
    homingDaggersMaxTotalTime = 0.0;
    enemiesAliveMaxTotal = 0;
    enemiesAliveMaxTotalTime = 0.0;
}
//
//void printTitle() {
//
//	// length = 66
//	attron(COLOR_PAIR(1));
//	mvprintw(1, (col - 66) / 2,  "@@@@@@@   @@@@@@@    @@@@@@   @@@@@@@   @@@@@@   @@@@@@@   @@@@@@ ");
//	mvprintw(2, (col - 66) / 2,  "@@@@@@@@  @@@@@@@@  @@@@@@@   @@@@@@@  @@@@@@@@  @@@@@@@  @@@@@@@ ");
//	mvprintw(3, (col - 66) / 2,  "@@!  @@@  @@!  @@@  !@@         @@!    @@!  @@@    @@!    !@@     ");
//	mvprintw(4, (col - 66) / 2,  "!@!  @!@  !@!  @!@  !@!         !@!    !@!  @!@    !@!    !@!     ");
//	mvprintw(5, (col - 66) / 2,  "@!@  !@!  @!@  !@!  !!@@!!      @!!    @!@!@!@!    @!!    !!@@!!  ");
//	mvprintw(6, (col - 66) / 2,  "!@!  !!!  !@!  !!!   !!@!!!     !!!    !!!@!!!!    !!!     !!@!!! ");
//	mvprintw(7, (col - 66) / 2,  "!!:  !!!  !!:  !!!       !:!    !!:    !!:  !!!    !!:         !:!");
//	mvprintw(8, (col - 66) / 2,  ":!:  !:!  :!:  !:!      !:!     :!:    :!:  !:!    :!:        !:! ");
//	mvprintw(9, (col - 66) / 2,  " :::: ::   :::: ::  :::: ::      ::    ::   :::     ::    :::: :: ");
//	mvprintw(10, (col - 66) / 2, ":: :  :   :: :  :   :: : :       :      :   : :     :     :: : : ");
//	mvprintw(11, ((col - 66) / 2) + (66 - version.length()-2), "v%s", version.c_str());
//	attroff(COLOR_PAIR(1));
//
//}
//

// Bintr's note: I'm commenting lines i deem bullshit and replacing them in this function
std::future<cpr::Response> sendToServer() {
    ///DWORD pointer;
    ///DWORD pTemp;
    ///DWORD pointerAddr;

    // get playerID if it hasn't been retreieved yet..
    ///if (playerID == -1) {
    ///    pointer = exeBaseAddress + playerIDBaseAddress;
    ///    if (!ReadProcessMemory(handle, (LPCVOID)pointer, &pTemp, sizeof(pTemp), NULL)) {
    ///        cout << " Failed to read address for playerID." << endl;
    ///    }
    ///    else {
    ///        pointerAddr = pTemp + playerIDOffset;
    ///        ReadProcessMemory(handle, (LPCVOID)pointerAddr, &playerID, sizeof(playerID), NULL);
    ///        gaPlayerID.getValue(playerID);
    ///    }
    ///}

    // this fixes the last captured time if user restarts to be accurate
    ///float inGameTimerFix;
    ///if (deathType == -1) {
    ///    inGameTimerFix = oldInGameTimer;
    ///}
    ///else {
    ///    inGameTimerFix = inGameTimer;
    ///}
    ///
    ///if (isReplay && !alive) {
    ///    if (replayUsernameMatch()) {
    ///        replayPlayerID = playerID;
    ///        strcpy_s(replayPlayerName, playerName);
    ///    }
    ///    else {
    ///        replayPlayerID = getReplayPlayerID(handle);
    ///    }
    ///}
    ///else {
    ///    replayPlayerID = 0;
    ///    strcpy_s(replayPlayerName, "none");
    ///}

    // Bintr: hash is already set when player moves to dagger lobby
    // if for some reason its not set it'll just be an empty string
    ///std::string survivalHash = getSurvivalHash();

    json log = {
        { "playerID"                , playerID                  },
        { "playerName"              , playerName                },
        { "granularity"             , interval                  },
        ///{ "inGameTimer"             , inGameTimerFix            },
        { "inGameTimer"             , timer                     },
        { "inGameTimerVector"       , inGameTimerVector         },
        ///{ "gems"                    , gems                      },
        { "gems"                    , totalGems                 },
        { "gemsVector"              , gemsVector                },
        ///{ "levelTwoTime"            , levelTwo                  },
        ///{ "levelThreeTime"          , levelThree                },
        ///{ "levelFourTime"           , levelFour                 },
        ///{ "homingDaggers"           , homingDaggers             },
        { "levelTwoTime"            , lvl2Time                  },
        { "levelThreeTime"          , lvl3Time                  },
        { "levelFourTime"           , lvl4Time                  },
        { "homingDaggers"           , homing                    },
        { "homingDaggersVector"     , homingDaggersVector       },
        { "homingDaggersMax"        , homingDaggersMaxTotal     },
        { "homingDaggersMaxTime"    , homingDaggersMaxTotalTime },
        { "daggersFired"            , daggersFired              },
        { "daggersFiredVector"      , daggersFiredVector        },
        { "daggersHit"              , daggersHit                },
        { "daggersHitVector"        , daggersHitVector          },
        { "enemiesAlive"            , enemiesAlive              },
        { "enemiesAliveVector"      , enemiesAliveVector        },
        { "enemiesAliveMax"         , enemiesAliveMaxTotal      },
        { "enemiesAliveMaxTime"     , enemiesAliveMaxTotalTime  },
        { "enemiesKilled"           , enemiesKilled             },
        { "enemiesKilledVector"     , enemiesKilledVector       },
        { "deathType"               , deathType                 },
        ///{ "replayPlayerID"          , replayPlayerID            },
        { "replayPlayerID"          , replayID                  },
        { "version"                 , version                   },
        ///{ "survivalHash"            , survivalHash              }
        { "survivalHash"            , currentHash               }
    };

    ///inGameTimerSubmit = inGameTimerFix;
    ///gemsSubmit = gems;
    ///homingDaggersSubmit = homingDaggers;
    ///daggersFiredSubmit = daggersFired;
    ///daggersHitSubmit = daggersHit;
    ///enemiesAliveSubmit = enemiesAlive;
    ///enemiesKilledSubmit = enemiesKilled;
    ///if (daggersFired == 0)
    ///    accuracySubmit = 0.0;
    ///else
    ///    accuracySubmit = (float)((daggersHit / daggersFired) * 100.0);
    ///levelTwoSubmit = levelTwo;
    ///levelThreeSubmit = levelThree;
    ///levelFourSubmit = levelFour;

    auto future_response = cpr::PostCallback([](cpr::Response r) {
        return r;
    },
        // cpr::Url{ "http://10.0.1.222:5666/api/submit_game" },
        cpr::Url{ "http://ddstats.com/api/submit_game" },
        cpr::Body{ log.dump() },
        cpr::Header{ { "Content-Type", "application/json" } });

    return future_response;

}
//
std::future<cpr::Response> getMOTD() {
    auto future_response = cpr::PostCallback([](cpr::Response r) {
        return r;
    },
        cpr::Url{ "http://ddstats.com/api/get_motd" },
        cpr::Body{ "{ \"version\": \"" + version + "\" }" },
        cpr::Header{ { "Content-Type", "application/json" } });
    return future_response;
}

// copied from getMOTD
std::future<cpr::Response> getChallengeSpawnset() {
    auto future_response = cpr::PostCallback([](cpr::Response r) {
        return r;
    },
        cpr::Url{ "http://ddstats.com/api/get_challenge_spawnset" },
        cpr::Body{ "{ \"version\": \"" + version + "\" }" },
        cpr::Header{ { "Content-Type", "application/json" } });
    return future_response;
} 

std::string getSurvivalHash() {
    wchar_t buf[MAX_PATH];
    if (handle) GetModuleFileNameExW(handle, 0, buf, MAX_PATH);
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

void setWindowSize(int width, int height) {
	HWND console = GetConsoleWindow();
	RECT r;
	GetWindowRect(console, &r);
	MoveWindow(console, r.left, r.top, width, height, TRUE); 
}
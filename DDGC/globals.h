#pragma once

#define say(x) std::cout << x << '\n'
#define writeVar(x) std::cout << #x": " << x << '\n'

// suggestion
auto future_response = std::future<cpr::Response>{};
auto future_motd_response = std::future<cpr::Response>{};
//std::future<cpr::Response> future_response = std::future<cpr::Response>{};
//std::future<cpr::Response> future_motd_response = std::future<cpr::Response>{};

auto future_challenge_spawnset = std::future<cpr::Response>{};

int timeStreamStats   = clock(); // stream packet
int timeLogSubmission = clock(); // full run log
int timeGameAvailable = clock();
int timeCopyMsg = clock();

std::string version = "0.3.2";
std::string title = "DDSTATS v" + version;

std::string msgSIO;
std::string msgGame;
std::string msgRecording;

std::string currentHash = "";

GameAddress gaGame(nullptr, 0x001F8084);
    GameAddress gaPlayerInfo(gaGame, 0x0);
        GameAddress gaPlayerPos(gaPlayerInfo, 0x18);
        GameAddress gaPlayerAim(gaPlayerInfo, 0x30);
        GameAddress gaHoming(gaPlayerInfo, 0x224);
        GameAddress gaGems(gaPlayerInfo, 0x218); // not total gems! see gaTotalGems below
        GameAddress gaLMB(gaPlayerInfo, 0xF8);

GameAddress gaGameStats(nullptr, 0x001F30C0);
    GameAddress gaTimer(gaGameStats, 0x1A0);
    GameAddress gaPlayerID(gaGameStats, 0x5C);
    GameAddress gaTotalGems(gaGameStats, 0x1C0);
    GameAddress gaDaggersFired(gaGameStats, 0x1B4);
    GameAddress gaDaggersHit(gaGameStats, 0x1B8);
    GameAddress gaEnemiesAlive(gaGameStats, 0x1FC);
    GameAddress gaEnemiesKilled(gaGameStats, 0x1BC);
    GameAddress gaDeathType(gaGameStats, 0x1C4);
    GameAddress gaIsAlive(gaGameStats, 0x1A4);
    GameAddress gaIsReplay(gaGameStats, 0x35D);
    GameAddress gaPlayerName(gaGameStats, 0x60);
    GameAddress gaReplayName(gaGameStats, 0x360);

// Replay ID String Chain
GameAddress gaRID1(nullptr, 0x001F80B0);
    GameAddress gaRID2(gaRID1, 0x0);
        GameAddress gaRID3(gaRID2, 0x18);
            GameAddress gaRID4(gaRID3, 0xC);
                GameAddress gaReplayIDStr(gaRID4, 0x4642);

std::string motd = "";


bool challengeMode = false; // true on successful download of daily challenge
bool downloading = false; // true while downloading challenge spawnset
bool isStreaming = true;
bool attached = false; // true if attached to Devil Daggers
bool running = true; // true while ddstats is running
bool showAliveStats = true;
bool showHelpMenu = false;
int playerID;
int replayID;
std::string playerName;
std::string replayName;
bool connected;
bool isReplay;
float timer     ;   int totalGems    ;
int daggersFired;   int homing       ;
int daggersHit  ;   int enemiesAlive ;
float accuracy  ;   int enemiesKilled;
        
bool isAlive;
bool deathType;
bool inMainMenu;
bool isOnline;
bool isPlaying;

bool playerJustDied = false;
bool justEnteredDaggerLobby = false;
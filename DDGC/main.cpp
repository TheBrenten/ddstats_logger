
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
#include "GameVar.h"
#include "KeyObj.h"
#include "globals.h"
#include "tools.h"
#include "survival_original.h"

using namespace std;
using json = nlohmann::json;




int main() {
    //replaceSpawnset(_acsurvival_original, 13784);
    sio::client h;
    h.connect("http://www.ddstats.com");
    future_motd_response = getMOTD();
    SetConsoleTitle(title.c_str());
	setWindowSize(640, 480);
    // loop while ddstats is running
    while (running) {
        // key inputs
        // if ddstats is active/focused window
        if (GetConsoleWindow() == GetForegroundWindow()) {
            // show help - F1
            if (kF1.justPressed()) {
                showHelpMenu = !showHelpMenu;
            }
            // toggle alive stats - F11
            if (kF11.justPressed()) {
                showAliveStats = !showAliveStats;
            }
            // close ddstats - F10 or alt+F4
            if (kF10.justPressed() ||
                (GetAsyncKeyState(VK_MENU) &&
                    GetAsyncKeyState(VK_F4))) {
                running = false;
            }
            // shift+F12 - toggle challenge spawnset
            // F12 - toggle stats streaming
            if (kF12.justPressed()) {
                if (GetAsyncKeyState(VK_LSHIFT)) {
                    if (challengeMode) {
                        challengeMode = false;

                    }
                    if (!downloading) {
                        downloading = true;
                        future_challenge_spawnset = getChallengeSpawnset();
                        say("[[ Downloading Challenge Spawnset for mm/dd/yy... ]]");
                    }
                }
                else {
                    isStreaming = !isStreaming;
                }

            }
        }

        // connect to DD if not already
        if (!FindWindow(NULL, "Devil Daggers"))
            attached = false;
        attached = attachToGame();

        // get values
        {
            // values to get regardless of DD running
            connected = h.opened();
            // MOTD (only retrieved once)
            if (future_motd_response.valid()) {
                auto msg = future_motd_response.get();
                motd = json::parse(msg.text).at("motd").get<string>();
            }

            // check if challenge spawnset finished
            
            //if (future_challenge_spawnset.valid()) {
            //    challengeMode = true;
            //    downloading = false;
            //    
            //    int spawnsetSize;
            //    unsigned char* challengeSpawnset = new unsigned char[spawnsetSize];
            //    //?????

            //    replaceSpawnset(challengeSpawnset, spawnsetSize);
            //    delete[] challengeSpawnset;
            //}

            // only get these if DD is running
            if (attached) {
                bool prevIsAlive = isAlive;
                bool prevEnemiesAlive = enemiesAlive;
                bool prevInMainMenu = inMainMenu;
                // get easy values
                gaPlayerID.getValue(playerID);
                gaIsReplay.getValue(isReplay);
                gaTimer.getValue(timer);
                gaTotalGems.getValue(totalGems);
                gaHoming.getValue(homing);
                gaEnemiesAlive.getValue(enemiesAlive);
                gaEnemiesKilled.getValue(enemiesKilled);
                gaIsAlive.getValue(isAlive);
                gaDeathType.getValue(deathType);
                gaPlayerName.getString(playerName);
                gaReplayName.getString(replayName);

                // get special values
                // accuracy
                gaDaggersFired.getValue(daggersFired);
                gaDaggersHit.getValue(daggersHit);
                if (daggersFired > 0) accuracy = (float)daggersHit / daggersFired * 100;
                else accuracy = 0;
                // replay ID
                char replayIDChars[10];
                gaReplayIDStr.getValue(replayIDChars);
                replayID = atoi(replayIDChars);


                // deduce from existing values
                if (isAlive) {
                    if (timer == 0.0) {
                        isPlaying = false;
                        inMainMenu = true;
                    }
                    else {
                        if (isReplay) isPlaying = false;
                        else isPlaying = true;
                        inMainMenu = false;
                    }
                }
                if (!isAlive && prevIsAlive) {
                    playerJustDied = true;
                }
                if (prevEnemiesAlive && prevInMainMenu && enemiesAlive == 0) {
                    justEnteredDaggerLobby = true;
                    say("Just entered dagger lobby from main menu!");
                }
            }
        }

        /*
        Submission handling!

        Submit full game log if
        -player just died
        -timer > 4
        -not inMainMenu (redundant - cant have timer > 4 in menu)
        
        Submit packet if
        -isStreaming stats
        -time since last packet > 0.5 seconds
        -player is alive/just died?
        */

        if (playerJustDied && timer > 4) {
            //say("Submitting scores!");
            playerJustDied = false;

        }

        if (clock() - timeStreamStats > 500 && 
            (isAlive || playerJustDied)) {
            timeStreamStats = clock();
            //say("Sending Stream Packet!");
        }
        // update md5 hash the instant player moves into dagger lobby
        if (justEnteredDaggerLobby) {
            currentHash = getSurvivalHash();
            justEnteredDaggerLobby = false;
        }
        //if (updateAvailable) {
        //    attron(COLOR_PAIR(2));
        //    mvprintw(rown, rightAlign - 18, "(UPDATE AVAILABLE)");
        //    attroff(COLOR_PAIR(2));
        //}

        if (showHelpMenu) printHelpMenu();

        writeVar(motd);
        writeVar(connected);
        writeVar(playerName);
        writeVar(replayName);
        writeVar(replayID);
        writeVar(playerID);

        writeVar(timer);
        writeVar(daggersFired);
        writeVar(daggersHit);
        writeVar(accuracy);
        writeVar(totalGems);
        writeVar(homing);
        writeVar(enemiesAlive);
        writeVar(enemiesKilled);

        writeVar(deathType);
        writeVar(isAlive);
        writeVar(isReplay);
        writeVar(inMainMenu);
        writeVar(isPlaying);
        writeVar(isStreaming);
        writeVar(showHelpMenu);
        writeVar(showAliveStats);
        writeVar(currentHash);


        // write 3 [[ status ]] lines
        {
            if (connected && isStreaming) {
                // color = green    
                msgSIO = "[[ Online ]]";
            }
            else {
                // color = red
                msgSIO = "[[ Offline ]]";
            }
            writeVar(msgSIO);

            if (attached) {
                // color = green
                msgGame = "[[ Connected to Devil Daggers ]]";
            }
            else {
                // color = red
                msgGame = "[[ Devil Daggers not found ]]";
            }
            writeVar(msgGame);

            if (timer != 0.0) {
                //attron(COLOR_PAIR(2)); // green
                msgRecording = "[[ Recording ]]";
            }
            else {
                //attron(COLOR_PAIR(1)); // red
                msgRecording = "[[ Not Recording ]]";
            }
            writeVar(msgRecording);
        }

        // adjust as needed - easier to read some things at 100
        Sleep(100);
        system("CLS");
    }
}

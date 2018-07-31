#include <string.h>
#include <stdio.h>
#include <dirent.h>
#include <string>
#include <iostream>
#include <fstream>
#include <ostream>
#include <sys/stat.h>
#include <math.h>
#include <vector>
#include <switch.h>
#include <chrono>

//TODO: massive refactor
//move everything out of main

struct cursorPos{
    int x;
    int y;
};

struct DirectoryEntry{
    int index;
    std::string filename;
    std::string fullpath;
    bool isfile;
    bool isempty;
};

typedef std::vector<DirectoryEntry> DirectoryList;

std::string tail(std::string const& source, size_t const length) {
  if (length >= source.size()) { return source; }
  return source.substr(source.size() - length);
}

bool is_file(const char* path) {
    struct stat buf;
    stat(path, &buf);
    return S_ISREG(buf.st_mode);
}

void copyFile(std::string srcPath, std::string dstPath, std::string deviceName){
    srcPath.append("/");
    std::ifstream src;
    std::ofstream dst;
    src.open(srcPath, std::ios::binary);
    dst.open(dstPath, std::ios::binary);
    
    dst << src.rdbuf();
    src.close();
    dst.close();
    //fsdevCommitDevice(deviceName.c_str());
}

void moveCursor(int y, int x){
    printf("\033[%d;%dH", (y), (x));
}

void clearScr(){
    printf("\e[1;1H\e[2J");
}

void printHeader(u64 emmcTotal, u64 emmcFree, u64 sdmcTotal, u64 sdmcFree){
    moveCursor(1, 0);
    printf("\033[0;31m");
    printf("partition total: ");
    printf("\033[0m");
    printf(std::to_string((int)trunc(emmcTotal / pow(1024, 2))).c_str());
    printf(" MB");
    printf("\033[0;31m");
    moveCursor(1, 50);
    printf("sdmc total: ");
    printf("\033[0m");
    printf(std::to_string((int)trunc(sdmcTotal / pow(1024, 2))).c_str());
    printf(" MB");
    moveCursor(1,31);
    printf("\033[0;32m");
    printf("kEx NAND Explorer");
    moveCursor(2,37);
    printf("\033[0;32m");
    printf("v0.04");
    moveCursor(2, 0);
    printf("\033[0;31m");
    printf("partition free: ");
    printf("\033[0m");
    printf(std::to_string((int)trunc(emmcFree / pow(1024, 2))).c_str());
    printf(" MB");
    moveCursor(2, 50);
    printf("\033[0;31m");
    printf("sdmc free: ");
    printf("\033[0m");
    printf(std::to_string((int)trunc(sdmcFree / pow(1024, 2))).c_str());
    printf(" MB");
    printf("\n");
    printf("\033[0;31m");
    printf("--------------------------------------------------------------------------------");
    printf("\033[0m");
    printf("\n");
}

void printMainMenu(){
    printf(" ");
    printf("SAFE:/");
    printf("\n");
    printf("  ");
    printf("SYSTEM:/");
    printf("\n");
    printf("  ");
    printf("USER:/");
    printf("\n");
    printf("  ");
    printf("SD:/");
}

void populateDirectory(std::string path, DirectoryList& dirList){

    struct dirent *ent;
    DIR* dir = opendir(path.c_str());
    int count = 1;
    
    dirList.clear();
    DirectoryEntry blankEntry;
    blankEntry.index = 0;
    blankEntry.isfile = false;
    blankEntry.filename = "..";
    blankEntry.isempty = true;
    blankEntry.fullpath = path;
    blankEntry.fullpath.append(blankEntry.filename);
    dirList.push_back(blankEntry);

    if(dir != NULL){
        
        while ((ent = readdir(dir)) != NULL) {
            
            std::string fname = ent->d_name;
            std::string fpath = path;
            fpath.append(fname);
            DirectoryEntry dirEntry;
            if(is_file(fpath.c_str()) == false){
                std::string fnameApp = fname;
                fnameApp.append("/");
                dirEntry.isfile = false;
            }
            else{
                dirEntry.isfile = true;
            }
            dirEntry.isempty = false;
            dirEntry.index = count;
            dirEntry.filename = fname;
            dirEntry.fullpath = path;
            dirEntry.fullpath.append(dirEntry.filename);
            dirList.push_back(dirEntry);
            count++;
            
        } 
        
    }
    closedir(dir);
}

void printDirectory(DirectoryList dirList, int screenSpace, int page){
    
    printf(dirList.at(0).fullpath.substr(0, dirList.at(0).fullpath.find_last_of("/") + 1).c_str());
    printf("\n");
    for(int i = (page*screenSpace); (i < (page + 1) * screenSpace); i++){
        if(i < dirList.size()){
            printf("  ");
            printf(dirList.at(i).filename.c_str());
            if(!dirList.at(i).isfile){
                printf("/");
            }
            printf("\n");
            
        }
    }
}

int main(int argc, char **argv)
{
    int currentMenuPage = 0;
    int screenSpace = 32;
    std::string copyPath;
    u64 emmcTotalSpace = 0;
    u64 emmcFreeSpace = 0;
    u64 sdmcTotalSpace = 0;
    u64 sdmcFreeSpace = 0;

    /* sloooow

    time_t curTime = time(NULL);
    struct tm* timerStartStruct = gmtime((const time_t *)&curTime);
    int timerStart = timerStartStruct->tm_sec;
    */

    std::chrono::milliseconds ms = std::chrono::duration_cast< std::chrono::milliseconds >(
        std::chrono::system_clock::now().time_since_epoch()
    );

    std::chrono::milliseconds msStart = std::chrono::duration_cast< std::chrono::milliseconds >(
        std::chrono::system_clock::now().time_since_epoch()
    );

    double timerDuration;

    int cursorOffset = 6;
    cursorPos currentCursorPos;
    cursorPos cursorHomePos;
    cursorHomePos.x = cursorOffset;
    cursorHomePos.y = 2;
    currentCursorPos.x = cursorOffset;
    currentCursorPos.y = 2;
    int cursorIndex = cursorHomePos.x - cursorOffset;
    bool inMainMenu = true;
    bool inDeletePrompt = false;

    std::string rootDeviceName = "";
    std::string rootDevicePath = rootDeviceName;
    rootDevicePath.append(":/");
    std::string currentDirPath = rootDevicePath;
    std::string currentFolder;
    std::string previousFolder;
    
    DirectoryList currentDirList;
    
    gfxInitDefault();
    consoleInit(NULL);

    DIR* dir;
    DIR* sdmcdir;
    struct dirent *ent;
    struct dirent *sdmcent;

    FsFileSystem emmcFs;

    bool emmcMountSucc;
    bool sdmcMountSucc = true;
    fsInitialize();

    const char *emmcStrBuff = "";
    FsFileSystem *sdmcFs = fsdevGetDefaultFileSystem();
    moveCursor(0, 0);
    fsFsGetFreeSpace(sdmcFs, "/", &sdmcFreeSpace);
    fsFsGetTotalSpace(sdmcFs, "/", &sdmcTotalSpace);
    printHeader(emmcTotalSpace, emmcFreeSpace, sdmcTotalSpace, sdmcFreeSpace);
    moveCursor(cursorHomePos.x, cursorHomePos.y);
    printMainMenu();   
        

    while(appletMainLoop()){
        hidScanInput();

        /*
        time_t curTime = time(NULL);
        struct tm* timerStartStruct = gmtime((const time_t *)&curTime);
        timerDuration = ( timerStartStruct->tm_sec - timerStart ) / (double) CLOCKS_PER_SEC;
        */
        
        ms = std::chrono::duration_cast< std::chrono::milliseconds >(
            std::chrono::system_clock::now().time_since_epoch()
        );

        timerDuration = (ms - msStart).count() / (double) CLOCKS_PER_SEC;

        moveCursor(currentCursorPos.x, currentCursorPos.y);
        printf(">");

        u64 kDown = hidKeysDown(CONTROLLER_P1_AUTO);
        u64 kHeld = hidKeysHeld(CONTROLLER_P1_AUTO);

        if(kHeld & KEY_DOWN){
            if(timerDuration >= 1){
                if(inMainMenu){
                    if((currentCursorPos.x + 1) <= 3 + cursorOffset){
                        moveCursor(currentCursorPos.x, currentCursorPos.y);
                        printf(" ");            
                        currentCursorPos.x++;
                        moveCursor(currentCursorPos.x, currentCursorPos.y);
                        printf(">");
                        cursorIndex = (currentCursorPos.x - cursorOffset) + (screenSpace * currentMenuPage);
                        msStart = std::chrono::duration_cast< std::chrono::milliseconds >(
                            std::chrono::system_clock::now().time_since_epoch()
                        );
                    }
                }

                else{
                    if(cursorIndex + 1 < currentDirList.size()){
                        if((currentCursorPos.x + 1) > screenSpace + cursorOffset - 1){
                            currentMenuPage++;
                            clearScr();
                            moveCursor(0, 0);
                            fsFsGetTotalSpace(&emmcFs, "/", &emmcTotalSpace);
                            fsFsGetTotalSpace(sdmcFs, "/", &sdmcTotalSpace);
                            fsFsGetFreeSpace(&emmcFs, "/", &emmcFreeSpace);
                            fsFsGetFreeSpace(sdmcFs, "/", &sdmcFreeSpace);
                            printHeader(emmcTotalSpace, emmcFreeSpace, sdmcTotalSpace, sdmcFreeSpace);
                            moveCursor(cursorHomePos.x - 1, cursorHomePos.y);
                            printDirectory(currentDirList, screenSpace, currentMenuPage);
                            currentCursorPos = cursorHomePos;
                            cursorIndex = (currentCursorPos.x - cursorOffset) + (screenSpace * currentMenuPage);
                            msStart = std::chrono::duration_cast< std::chrono::milliseconds >(
                                std::chrono::system_clock::now().time_since_epoch()
                            );
                            
                        }
                        else{
                            moveCursor(currentCursorPos.x, currentCursorPos.y);
                            printf(" ");            
                            currentCursorPos.x++;
                            moveCursor(currentCursorPos.x, currentCursorPos.y);
                            printf(">");
                            cursorIndex = (currentCursorPos.x - cursorOffset) + (screenSpace * currentMenuPage);
                            msStart = std::chrono::duration_cast< std::chrono::milliseconds >(
                                std::chrono::system_clock::now().time_since_epoch()
                            );
                        }
                    }
                }
            }
        }

        if(kHeld & KEY_UP){
            if(timerDuration >= 1){
                if((currentCursorPos.x - 1) > 0 + cursorOffset - 1){
                    moveCursor(currentCursorPos.x, currentCursorPos.y);
                    printf(" ");            
                    currentCursorPos.x--;
                    moveCursor(currentCursorPos.x, currentCursorPos.y);
                    printf(">");
                    cursorIndex = (currentCursorPos.x - cursorOffset) + (screenSpace * currentMenuPage);
                    msStart = std::chrono::duration_cast< std::chrono::milliseconds >(
                        std::chrono::system_clock::now().time_since_epoch()
                    );
                }
                else{
                    if(currentMenuPage > 0){
                        currentMenuPage--;
                        clearScr();
                        moveCursor(0, 0);
                        fsFsGetTotalSpace(&emmcFs, "/", &emmcTotalSpace);
                        fsFsGetTotalSpace(sdmcFs, "/", &sdmcTotalSpace);
                        fsFsGetFreeSpace(&emmcFs, "/", &emmcFreeSpace);
                        fsFsGetFreeSpace(sdmcFs, "/", &sdmcFreeSpace);
                        printHeader(emmcTotalSpace, emmcFreeSpace, sdmcTotalSpace, sdmcFreeSpace);
                        moveCursor(cursorHomePos.x - 1, cursorHomePos.y);
                        printDirectory(currentDirList, screenSpace, currentMenuPage);
                        cursorIndex = (currentCursorPos.x - cursorOffset) + (screenSpace * currentMenuPage);
                        msStart = std::chrono::duration_cast< std::chrono::milliseconds >(
                            std::chrono::system_clock::now().time_since_epoch()
                        );
                    }
                }
            }
        }
        
        if(kDown & KEY_A){

            if(inMainMenu){
                switch(cursorIndex){
                    case 0: 
                        if(rootDeviceName.compare("sdmc") != 0){
                            fsdevUnmountDevice(rootDeviceName.c_str());
                        }
                        rootDeviceName = "kSAFE";
                        rootDevicePath = rootDeviceName;
                        rootDevicePath.append(":/");
                        currentDirPath = rootDevicePath; 
                        fsOpenBisFileSystem(&emmcFs, 29, emmcStrBuff);
                        fsdevMountDevice(rootDeviceName.c_str(), emmcFs);
                        clearScr();
                        moveCursor(0, 0);
                        fsFsGetTotalSpace(&emmcFs, "/", &emmcTotalSpace);
                        fsFsGetTotalSpace(sdmcFs, "/", &sdmcTotalSpace);
                        fsFsGetFreeSpace(&emmcFs, "/", &emmcFreeSpace);
                        fsFsGetFreeSpace(sdmcFs, "/", &sdmcFreeSpace);
                        printHeader(emmcTotalSpace, emmcFreeSpace, sdmcTotalSpace, sdmcFreeSpace);
                        moveCursor(cursorHomePos.x - 1, cursorHomePos.y);
                        currentMenuPage = 0;
                        populateDirectory(currentDirPath, currentDirList);
                        printDirectory(currentDirList, screenSpace, currentMenuPage);
                        moveCursor(cursorHomePos.x, cursorHomePos.y);
                        currentCursorPos = cursorHomePos;
                        inMainMenu = !inMainMenu;
                        cursorIndex = (currentCursorPos.x - cursorOffset) + (screenSpace * currentMenuPage);
                        break;

                    case 1:
                        if(rootDeviceName.compare("sdmc") != 0){
                            fsdevUnmountDevice(rootDeviceName.c_str());
                        }
                        rootDeviceName = "kSYSTEM";
                        rootDevicePath = rootDeviceName;
                        rootDevicePath.append(":/");
                        currentDirPath = rootDevicePath; 
                        fsOpenBisFileSystem(&emmcFs, 31, emmcStrBuff);
                        fsdevMountDevice(rootDeviceName.c_str(), emmcFs);
                        clearScr();
                        moveCursor(0, 0);
                        fsFsGetTotalSpace(&emmcFs, "/", &emmcTotalSpace);
                        fsFsGetTotalSpace(sdmcFs, "/", &sdmcTotalSpace);
                        fsFsGetFreeSpace(&emmcFs, "/", &emmcFreeSpace);
                        fsFsGetFreeSpace(sdmcFs, "/", &sdmcFreeSpace);
                        printHeader(emmcTotalSpace, emmcFreeSpace, sdmcTotalSpace, sdmcFreeSpace);
                        moveCursor(cursorHomePos.x - 1, cursorHomePos.y);
                        currentMenuPage = 0;
                        populateDirectory(currentDirPath, currentDirList);
                        printDirectory(currentDirList, screenSpace, currentMenuPage);
                        moveCursor(cursorHomePos.x, cursorHomePos.y);
                        currentCursorPos = cursorHomePos;
                        inMainMenu = !inMainMenu;
                        cursorIndex = (currentCursorPos.x - cursorOffset) + (screenSpace * currentMenuPage);
                        break;
                    case 2:
                        if(rootDeviceName.compare("sdmc") != 0){
                            fsdevUnmountDevice(rootDeviceName.c_str());
                        }
                        rootDeviceName = "kUSER";
                        rootDevicePath = rootDeviceName;
                        rootDevicePath.append(":/");
                        currentDirPath = rootDevicePath; 
                        fsOpenBisFileSystem(&emmcFs, 30, emmcStrBuff);
                        fsdevMountDevice(rootDeviceName.c_str(), emmcFs);
                        clearScr();
                        moveCursor(0, 0);
                        fsFsGetTotalSpace(&emmcFs, "/", &emmcTotalSpace);
                        fsFsGetTotalSpace(sdmcFs, "/", &sdmcTotalSpace);
                        fsFsGetFreeSpace(&emmcFs, "/", &emmcFreeSpace);
                        fsFsGetFreeSpace(sdmcFs, "/", &sdmcFreeSpace);
                        printHeader(emmcTotalSpace, emmcFreeSpace, sdmcTotalSpace, sdmcFreeSpace);
                        moveCursor(cursorHomePos.x - 1, cursorHomePos.y);
                        currentMenuPage = 0;
                        populateDirectory(currentDirPath, currentDirList);
                        printDirectory(currentDirList, screenSpace, currentMenuPage);
                        moveCursor(cursorHomePos.x, cursorHomePos.y);
                        currentCursorPos = cursorHomePos;
                        inMainMenu = !inMainMenu;
                        cursorIndex = (currentCursorPos.x - cursorOffset) + (screenSpace * currentMenuPage);
                        break;
                    case 3:
                        rootDeviceName = "sdmc";
                        rootDevicePath = rootDeviceName;
                        rootDevicePath.append(":/");
                        currentDirPath = rootDevicePath; 
                        clearScr();
                        moveCursor(0, 0);
                        fsFsGetTotalSpace(sdmcFs, "/", &sdmcTotalSpace);
                        fsFsGetFreeSpace(sdmcFs, "/", &sdmcFreeSpace);
                        printHeader(emmcTotalSpace, emmcFreeSpace, sdmcTotalSpace, sdmcFreeSpace);
                        moveCursor(cursorHomePos.x - 1, cursorHomePos.y);
                        populateDirectory(currentDirPath, currentDirList);
                        currentMenuPage = 0;
                        printDirectory(currentDirList, screenSpace, currentMenuPage);
                        moveCursor(cursorHomePos.x, cursorHomePos.y);
                        currentCursorPos = cursorHomePos;
                        inMainMenu = !inMainMenu;
                        cursorIndex = (currentCursorPos.x - cursorOffset) + (screenSpace * currentMenuPage);
                        break;
                }
            }
            
            else if(!inMainMenu){

                if(!currentDirList.at(cursorIndex).isempty){
                    if(!currentDirList.at(cursorIndex).isfile){

                        clearScr();
                        currentFolder = currentDirList.at(cursorIndex).filename;

                        currentDirPath.append(currentDirList.at(cursorIndex).filename);
                        moveCursor(cursorOffset, 2);
                        currentDirPath.append("/");

                        clearScr();
                        moveCursor(0, 0);

                        fsFsGetTotalSpace(&emmcFs, "/", &emmcTotalSpace);
                        fsFsGetTotalSpace(sdmcFs, "/", &sdmcTotalSpace);
                        fsFsGetFreeSpace(&emmcFs, "/", &emmcFreeSpace);
                        fsFsGetFreeSpace(sdmcFs, "/", &sdmcFreeSpace);
                        printHeader(emmcTotalSpace, emmcFreeSpace, sdmcTotalSpace, sdmcFreeSpace);
                        moveCursor(cursorHomePos.x - 1, cursorHomePos.y);
                        populateDirectory(currentDirPath, currentDirList);
                        currentMenuPage = 0;
                        printDirectory(currentDirList, screenSpace, currentMenuPage);
                        currentCursorPos = cursorHomePos;
                        cursorIndex = (currentCursorPos.x - cursorOffset) + (screenSpace * currentMenuPage);
                    }
                }
            }
        }

        if(kDown & KEY_B){
            if(currentDirPath.compare(rootDevicePath) != 0){
                currentDirPath = currentDirPath.substr(0, currentDirPath.find_last_of("/"));
                currentDirPath = currentDirPath.substr(0, currentDirPath.find_last_of("/"));
                currentDirPath.append("/");
                clearScr();
                moveCursor(0, 0);
                fsFsGetTotalSpace(&emmcFs, "/", &emmcTotalSpace);
                fsFsGetTotalSpace(sdmcFs, "/", &sdmcTotalSpace);
                fsFsGetFreeSpace(&emmcFs, "/", &emmcFreeSpace);
                fsFsGetFreeSpace(sdmcFs, "/", &sdmcFreeSpace);
                printHeader(emmcTotalSpace, emmcFreeSpace, sdmcTotalSpace, sdmcFreeSpace);
                moveCursor(cursorHomePos.x - 1, cursorHomePos.y);
                populateDirectory(currentDirPath, currentDirList);
                currentMenuPage = 0;
                printDirectory(currentDirList, screenSpace, currentMenuPage);
                cursorIndex = cursorHomePos.x - cursorOffset;
                moveCursor(cursorHomePos.y, cursorHomePos.x);
                currentCursorPos = cursorHomePos;
                cursorIndex = (currentCursorPos.x - cursorOffset) + (screenSpace * currentMenuPage);
            }

            else{
                if(!inMainMenu){
                    clearScr();
                    moveCursor(0, 0);
                    fsFsGetTotalSpace(sdmcFs, "/", &sdmcTotalSpace);
                    fsFsGetFreeSpace(sdmcFs, "/", &sdmcFreeSpace);
                    fsFsGetTotalSpace(&emmcFs, "/", &emmcTotalSpace);
                    fsFsGetFreeSpace(&emmcFs, "/", &emmcFreeSpace);
                    printHeader(emmcTotalSpace, emmcFreeSpace, sdmcTotalSpace, sdmcFreeSpace);
                    moveCursor(cursorHomePos.x, cursorHomePos.y);
                    printMainMenu();
                    currentMenuPage = 0;
                    cursorIndex = cursorHomePos.x - cursorOffset;
                    moveCursor(cursorHomePos.x, cursorHomePos.y);
                    currentCursorPos = cursorHomePos;
                    cursorIndex = (currentCursorPos.x - cursorOffset) + (screenSpace * currentMenuPage);
                    inMainMenu = !inMainMenu;
                }
            }
        }

        if(kDown & KEY_X){
            if(!inMainMenu){
                if(!currentDirList.at(cursorIndex).isempty){
                    if(currentDirList.at(cursorIndex).isfile){
                        copyPath = currentDirPath;
                        copyPath.append(currentDirList.at(cursorIndex).filename);
                    }
                }
            }
        }

        if(kDown & KEY_Y){            
            if(!inMainMenu){
                std::string dstPath;
                dstPath = currentDirPath;
                dstPath.append(currentDirList.at(cursorIndex).filename);
                dstPath = dstPath.substr(0, dstPath.find_last_of("/") + 1);
                if(copyPath.substr(0, copyPath.find_last_of("/") + 1).compare(dstPath) != 0){
                    dstPath.append(copyPath.substr(copyPath.find_last_of("/") + 1));
                    copyFile(copyPath, dstPath, rootDeviceName);
                    clearScr();
                    moveCursor(0, 0);
                    fsFsGetTotalSpace(&emmcFs, "/", &emmcTotalSpace);
                    fsFsGetTotalSpace(sdmcFs, "/", &sdmcTotalSpace);
                    fsFsGetFreeSpace(&emmcFs, "/", &emmcFreeSpace);
                    fsFsGetFreeSpace(sdmcFs, "/", &sdmcFreeSpace);
                    printHeader(emmcTotalSpace, emmcFreeSpace, sdmcTotalSpace, sdmcFreeSpace);
                    populateDirectory(currentDirPath, currentDirList);
                    printDirectory(currentDirList, screenSpace, currentMenuPage);
                    moveCursor(4, 2);
                    printf("Copied!");
                    cursorIndex = (currentCursorPos.x - cursorOffset) + (screenSpace * currentMenuPage);
                    currentCursorPos = cursorHomePos;
                }
            }
        }

        if(kDown & KEY_MINUS){
            if(!inMainMenu){
                if(!currentDirList.at(cursorIndex).isempty){
                    if(currentDirList.at(cursorIndex).isfile){
                        inDeletePrompt = true;
                        moveCursor(5, 50);
                        printf("______________________________");
                        moveCursor(6, 50);
                        printf("|                            |");
                        moveCursor(7, 50);
                        printf("|         DELETE FILE        |");
                        moveCursor(8, 50);
                        printf("|        Are you sure?       |");
                        moveCursor(9, 50);
                        printf("|Press the A Button to delete|");
                        moveCursor(10, 50);
                        printf("| Any other button to cancel |");
                        moveCursor(11, 50);
                        printf("|____________________________|");
                        moveCursor(currentCursorPos.y, currentCursorPos.x);
                        while(inDeletePrompt){
                            gfxFlushBuffers();
                            gfxSwapBuffers();
                            gfxWaitForVsync();

                            hidScanInput();
                            u64 kDown = hidKeysDown(CONTROLLER_P1_AUTO);

                            if(kDown & KEY_A){
                                std::string delPath;
                                delPath = currentDirPath;
                                delPath.append(currentDirList.at(cursorIndex).filename);
                                remove(delPath.c_str());
                                clearScr();
                                moveCursor(0, 0);
                                fsFsGetTotalSpace(&emmcFs, "/", &emmcTotalSpace);
                                fsFsGetTotalSpace(sdmcFs, "/", &sdmcTotalSpace);
                                fsFsGetFreeSpace(&emmcFs, "/", &emmcFreeSpace);
                                fsFsGetFreeSpace(sdmcFs, "/", &sdmcFreeSpace);
                                printHeader(emmcTotalSpace, emmcFreeSpace, sdmcTotalSpace, sdmcFreeSpace);
                                populateDirectory(currentDirPath, currentDirList);

                                printDirectory(currentDirList, screenSpace, currentMenuPage);
                                moveCursor(4, 2);
                                printf("Deleted: ");
                                printf(delPath.c_str());
                                currentCursorPos = cursorHomePos;
                                cursorIndex = (currentCursorPos.x - cursorOffset) + (screenSpace * currentMenuPage);
                                inDeletePrompt = !inDeletePrompt;
                                break;
                            }
                            else if(kDown && !(kDown & KEY_A)){
                                clearScr();
                                moveCursor(0, 0);
                                printHeader(emmcTotalSpace, emmcFreeSpace, sdmcTotalSpace, sdmcFreeSpace);
                                populateDirectory(currentDirPath, currentDirList);
                                printDirectory(currentDirList, screenSpace, currentMenuPage);
                                moveCursor(4, 2);
                                printf("Cancelled!");
                                currentCursorPos = cursorHomePos;
                                cursorIndex = (currentCursorPos.x - cursorOffset) + (screenSpace * currentMenuPage);
                                inDeletePrompt = !inDeletePrompt;
                                break;
                            }
                        }
                    }
                }
            }
        }

        if(kDown & KEY_PLUS){
            fsdevUnmountDevice(rootDeviceName.c_str());
            fsdevUnmountAll();
            break;
        }
        
        gfxFlushBuffers();
        gfxSwapBuffers();
        gfxWaitForVsync();
    }

    gfxExit();
    return 0;
    
}


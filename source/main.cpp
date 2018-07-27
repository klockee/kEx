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

//TODO: massive cleanup
//move everything out of main
//fix menu drawing
//general improvements

struct cursorPos{
    int x;
    int y;
};

struct DirectoryEntry{
    int index;
    std::string path;
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
    printf("v0.03");
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

void printDirectory(std::string path, DirectoryList& dirList){

    struct dirent *ent;
    printf(path.c_str());
    printf("\n");
    DIR* dir = opendir(path.c_str());
    int count = 1;
    
    dirList.clear();
    DirectoryEntry blankEntry;
    blankEntry.index = 0;
    blankEntry.isfile = false;
    blankEntry.path = "..";
    blankEntry.isempty = true;
    dirList.push_back(blankEntry);
    printf("  ");
    printf(blankEntry.path.c_str());
    printf("\n");

    if(dir != NULL){
        
        while ((ent = readdir(dir)) != NULL) {
            
            std::string fname = ent->d_name;
            std::string fpath = path;
            fpath.append(fname);
            DirectoryEntry dirEntry;
            printf("  ");
            if(is_file(fpath.c_str()) == false){
                std::string fnameApp = fname;
                fnameApp.append("/");
                printf(fnameApp.c_str());
                dirEntry.isfile = false;
            }
            else{
                printf(fname.c_str());
                dirEntry.isfile = true;
            }
            printf("\n");
            dirEntry.isempty = false;

            dirEntry.index = count;
            dirEntry.path = fname;
            dirList.push_back(dirEntry);
            count++;
            
        } 
        
    }
    closedir(dir);
}

int main(int argc, char **argv)
{

    std::string copyPath;
    u64 emmcTotalSpace = 0;
    u64 emmcFreeSpace = 0;
    u64 sdmcTotalSpace = 0;
    u64 sdmcFreeSpace = 0;

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

        moveCursor(currentCursorPos.x, currentCursorPos.y);
        printf(">");

        u64 kDown = hidKeysDown(CONTROLLER_P1_AUTO);

        if(kDown & KEY_DOWN){
            if(inMainMenu){
                if((currentCursorPos.x + 1) <= 3 + cursorOffset){
                    moveCursor(currentCursorPos.x, currentCursorPos.y);
                    printf(" ");            
                    currentCursorPos.x++;
                    moveCursor(currentCursorPos.x, currentCursorPos.y);
                    printf(">");
                    cursorIndex = currentCursorPos.x - cursorOffset;
                }
            }

            else{
                if((currentCursorPos.x + 1) <= currentDirList.size() + cursorOffset - 1){
                    moveCursor(currentCursorPos.x, currentCursorPos.y);
                    printf(" ");            
                    currentCursorPos.x++;
                    moveCursor(currentCursorPos.x, currentCursorPos.y);
                    printf(">");
                    cursorIndex = currentCursorPos.x - cursorOffset;
                }
            }
        }

        if(kDown & KEY_UP){
            if((currentCursorPos.x - 1) > 0 + cursorOffset - 1){
                moveCursor(currentCursorPos.x, currentCursorPos.y);
                printf(" ");            
                currentCursorPos.x--;
                moveCursor(currentCursorPos.x, currentCursorPos.y);
                printf(">");
                cursorIndex = currentCursorPos.x - cursorOffset;
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
                        printDirectory(currentDirPath, currentDirList);
                        moveCursor(cursorHomePos.x, cursorHomePos.y);
                        currentCursorPos = cursorHomePos;
                        inMainMenu = !inMainMenu;
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
                        printDirectory(currentDirPath, currentDirList);
                        moveCursor(cursorHomePos.x, cursorHomePos.y);
                        currentCursorPos = cursorHomePos;
                        inMainMenu = !inMainMenu;
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
                        printDirectory(currentDirPath, currentDirList);
                        moveCursor(cursorHomePos.x, cursorHomePos.y);
                        currentCursorPos = cursorHomePos;
                        inMainMenu = !inMainMenu;
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
                        printDirectory(currentDirPath, currentDirList);
                        moveCursor(cursorHomePos.x, cursorHomePos.y);
                        currentCursorPos = cursorHomePos;
                        inMainMenu = !inMainMenu;
                        break;
                }
            }
            
            else if(!inMainMenu){

                if(!currentDirList.at(currentCursorPos.x - cursorOffset).isempty){
                    if(!currentDirList.at(currentCursorPos.x - cursorOffset).isfile){

                        clearScr();
                        currentFolder = currentDirList.at(currentCursorPos.x - cursorOffset).path;

                        currentDirPath.append(currentDirList.at(currentCursorPos.x - cursorOffset).path);
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
                        printDirectory(currentDirPath, currentDirList);
                        cursorIndex = cursorHomePos.x - cursorOffset;
                        currentCursorPos = cursorHomePos;
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
                printDirectory(currentDirPath, currentDirList);
                cursorIndex = cursorHomePos.x - cursorOffset;
                moveCursor(cursorHomePos.y, cursorHomePos.x);
                currentCursorPos = cursorHomePos;
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
                    cursorIndex = cursorHomePos.x - cursorOffset;
                    moveCursor(cursorHomePos.x, cursorHomePos.y);
                    currentCursorPos = cursorHomePos;
                    inMainMenu = !inMainMenu;
                }
            }
        }

        if(kDown & KEY_X){
            if(!inMainMenu){
                if(!currentDirList.at(currentCursorPos.x - cursorOffset).isempty){
                    if(currentDirList.at(currentCursorPos.x - cursorOffset).isfile){
                        copyPath = currentDirPath;
                        copyPath.append(currentDirList.at(currentCursorPos.x - cursorOffset).path);
                    }
                }
            }
        }

        if(kDown & KEY_Y){            
            if(!inMainMenu){
                std::string dstPath;
                dstPath = currentDirPath;
                dstPath.append(currentDirList.at(currentCursorPos.x - cursorOffset).path);
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
                    printDirectory(currentDirPath, currentDirList);
                    moveCursor(4, 2);
                    printf("Copied!");
                    cursorIndex = cursorHomePos.x - cursorOffset;
                    currentCursorPos = cursorHomePos;
                }
            }
        }

        if(kDown & KEY_MINUS){
            if(!inMainMenu){
                if(!currentDirList.at(currentCursorPos.x - cursorOffset).isempty){
                    if(currentDirList.at(currentCursorPos.x - cursorOffset).isfile){
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
                                delPath.append(currentDirList.at(currentCursorPos.x - cursorOffset).path);
                                remove(delPath.c_str());
                                clearScr();
                                moveCursor(0, 0);
                                fsFsGetTotalSpace(&emmcFs, "/", &emmcTotalSpace);
                                fsFsGetTotalSpace(sdmcFs, "/", &sdmcTotalSpace);
                                fsFsGetFreeSpace(&emmcFs, "/", &emmcFreeSpace);
                                fsFsGetFreeSpace(sdmcFs, "/", &sdmcFreeSpace);
                                printHeader(emmcTotalSpace, emmcFreeSpace, sdmcTotalSpace, sdmcFreeSpace);
                                printDirectory(currentDirPath, currentDirList);
                                moveCursor(4, 2);
                                printf("Deleted: ");
                                printf(delPath.c_str());
                                cursorIndex = cursorHomePos.x - cursorOffset;
                                currentCursorPos = cursorHomePos;
                                inDeletePrompt = !inDeletePrompt;
                                break;
                            }
                            else if(kDown && !(kDown & KEY_A)){
                                clearScr();
                                moveCursor(0, 0);
                                printHeader(emmcTotalSpace, emmcFreeSpace, sdmcTotalSpace, sdmcFreeSpace);
                                printDirectory(currentDirPath, currentDirList);
                                moveCursor(4, 2);
                                printf("Cancelled!");
                                cursorIndex = cursorHomePos.x - cursorOffset;
                                currentCursorPos = cursorHomePos;
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


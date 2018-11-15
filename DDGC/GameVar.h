#pragma once

HANDLE handle = NULL;
uintptr_t exeBaseAddress = 0;

class GameAddress {
    // if parent is null, use exe base address
    GameAddress* parent = nullptr;
    int offset = 0;

    DWORD getBaseAddress() {
        DWORD base;
        if (parent) {
            parent->getValue(base);
        }
        else {
            base = uintptr_t(exeBaseAddress);
        }
        return base;
    }

public:
    GameAddress(GameAddress &parent, int offset) {
        this->parent = &parent;
        this->offset = offset;
    }
    GameAddress(GameAddress* parent, int offset) {
        this->parent = parent;
        this->offset = offset;
    }

    template<typename T> void getValue(T &val) {
        try {
            DWORD address = getBaseAddress() + offset;
            ReadProcessMemory(handle, (LPCVOID)address, &val, sizeof(val), NULL);
        }
        catch (...) {
            // set all the bytes of val to 0
            for (int i = 0; i < sizeof(T); i++) {
                char* ptr = static_cast<char*>((void*)(&val)) + i;
                *ptr = '\0';
                //std::cout << (void*)ptr << std::endl; // print the addresses
                //Sleep(1000);
            }
        }
    }
    template<typename T> void setValue(T& val) {
        try {
            DWORD address = getBaseAddress() + offset;
            WriteProcessMemory(handle, (LPVOID)address, &val, sizeof(val), 0);
        }
        catch (...) {}
    }

    void getString(std::string& str) {
        char* charArray = nullptr;
        try {
            DWORD address = getBaseAddress() + offset;
            DWORD size_address = address + 0x10;
            int size;

            ReadProcessMemory(handle, (LPCVOID)size_address, &size, sizeof(size), NULL);
            size++; // for null terminator
            charArray = new char[size];
            DWORD alloc_address = address + 0x14;
            int sizeToAlloc;
            ReadProcessMemory(handle, (LPCVOID)alloc_address, &sizeToAlloc, sizeof(sizeToAlloc), NULL);
            // adjust address
            if (sizeToAlloc > 15)
                ReadProcessMemory(handle, (LPCVOID)address, &address, sizeof(address), NULL);
            ReadProcessMemory(handle, (LPCVOID)address, charArray, size, NULL);
            str = charArray;
        }
        catch (...) {
            str = "";
        }
        // free memory regardless of exceptions
        if (charArray)
            delete[] charArray;
    }

};


// returns true on success
bool attachToGame() {
    HWND hwnd = FindWindow(NULL, "Devil Daggers");
    if (hwnd == NULL) {
        //std::cout << "Can't find window\n";
        return false;
    }
    DWORD procID = NULL;
    GetWindowThreadProcessId(hwnd, &procID);
    handle = OpenProcess(PROCESS_ALL_ACCESS, FALSE, procID);

    if (procID == NULL) {
        //std::cout << "Couldn't obtain process" << std::endl;
        return false;
    }
    exeBaseAddress = 0;
    HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, procID);
    if (hSnap != INVALID_HANDLE_VALUE) {
        MODULEENTRY32 modEntry;
        modEntry.dwSize = sizeof(modEntry);
        // find dd.exe base
        if (Module32First(hSnap, &modEntry)) {
			exeBaseAddress = (uintptr_t)modEntry.modBaseAddr;
        }
    }
    CloseHandle(hSnap);
    //std::cout << "Found Devil Daggers window!" << std::endl;
    return true;
}

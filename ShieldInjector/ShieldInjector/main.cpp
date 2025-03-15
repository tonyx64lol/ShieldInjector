#include <iostream>
#include <windows.h>
#include <tlhelp32.h>
#include <string>
#include <thread>
#include <chrono>
#include <vector>

//    ShieldInjector - Secure & Undetected DLL injector with a sleek CLI
//    Author: Tony Davis
//    GitHub: https://github.com/tonyx64lol/ShieldInjector
//
//    Description:
//    This project is a DLL injector designed for educational and research purposes only.
//    Unauthorized use of this software for malicious purposes is strictly prohibited.
//
//    Credits:
//    - tonyx64lol
//
//    Last Updated: 15/03/2025

#define RESET   "\033[0m"
#define BLACK   "\033[30m"
#define RED     "\033[31m"
#define GREEN   "\033[32m"
#define YELLOW  "\033[33m"
#define BLUE    "\033[34m"
#define MAGENTA "\033[35m"
#define CYAN    "\033[36m"
#define WHITE   "\033[37m"
#define GRAY    "\033[90m"

#define L_YELLOW "\033[93m"
#define L_GREEN "\033[92m"

std::string generateRandomString(int length) {
    std::string randomStr;
    const char charset[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";
    const int charsetSize = sizeof(charset) - 1;

    for (int i = 0; i < length; i++) {
        randomStr += charset[rand() % charsetSize];
    }
    return randomStr;
}

void updateTitle() {
    srand(static_cast<unsigned int>(time(0)));

    while (true) {
        std::string randomPart = generateRandomString(5);
        std::string newTitle = "ShieldInjector - by Tony | " + randomPart;
        SetConsoleTitleA(newTitle.c_str());
        std::this_thread::sleep_for(std::chrono::milliseconds(400));
    }
}

void setConsoleColor(bool injected) {
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    SetConsoleTextAttribute(hConsole, injected ? FOREGROUND_GREEN : FOREGROUND_RED);
}

std::vector<DWORD> getProcessIdsByName(const std::wstring& processName) {
    std::vector<DWORD> processIDs;
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);

    if (snapshot == INVALID_HANDLE_VALUE) {
        return processIDs;
    }

    PROCESSENTRY32W procEntry;
    procEntry.dwSize = sizeof(PROCESSENTRY32W);

    if (Process32FirstW(snapshot, &procEntry)) {
        do {
            if (processName == procEntry.szExeFile) {
                processIDs.push_back(procEntry.th32ProcessID);
            }
        } while (Process32NextW(snapshot, &procEntry));
    }

    CloseHandle(snapshot);
    return processIDs;
}

bool injectDLL(DWORD processID, const std::string& dllPath) {
    HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, processID);
    if (!hProcess) {
        DWORD error = GetLastError();
        if (error == ERROR_ACCESS_DENIED) {
            std::cerr << "[0x1] Access Denied (Run as Administrator)\n";
        }
        else {
            std::cerr << "Failed to open process: " << error << "\n";
        }
        return false;
    }

    LPVOID allocMem = VirtualAllocEx(hProcess, nullptr, dllPath.size() + 1, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    if (!allocMem) {
        std::cerr << "Failed to allocate memory: " << GetLastError() << "\n";
        CloseHandle(hProcess);
        return false;
    }

    if (!WriteProcessMemory(hProcess, allocMem, dllPath.c_str(), dllPath.size() + 1, nullptr)) {
        std::cerr << "Failed to write process memory: " << GetLastError() << "\n";
        VirtualFreeEx(hProcess, allocMem, 0, MEM_RELEASE);
        CloseHandle(hProcess);
        return false;
    }

    HANDLE hThread = CreateRemoteThread(hProcess, nullptr, 0,
        reinterpret_cast<LPTHREAD_START_ROUTINE>(&LoadLibraryA), allocMem, 0, nullptr);
    if (!hThread) {
        std::cerr << "Failed to create remote thread: " << GetLastError() << "\n";
        VirtualFreeEx(hProcess, allocMem, 0, MEM_RELEASE);
        CloseHandle(hProcess);
        return false;
    }

    WaitForSingleObject(hThread, INFINITE);
    VirtualFreeEx(hProcess, allocMem, 0, MEM_RELEASE);
    CloseHandle(hThread);
    CloseHandle(hProcess);
    return true;
}

void enableANSI() {
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD dwMode = 0;
    GetConsoleMode(hOut, &dwMode);
    SetConsoleMode(hOut, dwMode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
}

int main() {
    enableANSI();

    std::thread titleThread(updateTitle);
    titleThread.detach();

    std::string input;
    DWORD processID = 0;
    std::string dllPath;
    int delay;

    std::cout << L_YELLOW << "ShieldInjector - " << GRAY << "Secure & Undetected DLL injector\n\n";
    std::cout << "Enter PID or .exe name: " << RESET;
    std::cin >> input;

    if (isdigit(input[0])) {
        processID = std::stoi(input);
    }
    else {
        std::vector<DWORD> processIDs = getProcessIdsByName(std::wstring(input.begin(), input.end()));

        if (processIDs.empty()) {
            std::cerr << RED << "Process not found!\n";
            Sleep(3000);
            return 1;
        }

        if (processIDs.size() > 1) {
            std::cout << "Multiple processes found. Select one:\n";
            for (size_t i = 0; i < processIDs.size(); ++i) {
                std::cout << i + 1 << ": " << processIDs[i] << "\n";
            }
            std::cout << GRAY << "Enter your choice: " << RESET;
            int choice;
            std::cin >> choice;
            if (choice < 1 || choice > processIDs.size()) {
                std::cerr << "Invalid selection.\n";
                return 1;
            }
            processID = processIDs[choice - 1];
        }
        else {
            processID = processIDs[0];
        }
    }

    std::cout << GRAY << "Enter DLL Path: " << RESET;
    std::cin >> dllPath;


    if (injectDLL(processID, dllPath)) {
        std::cout << "\n\nStatus: " << L_GREEN << "Injected";
    }
    else {
        std::cout << "\n\nStatus: " << RED << "Not Injected";
    }

    while (true) {
        Sleep(1000);
    }

    return 0;
}
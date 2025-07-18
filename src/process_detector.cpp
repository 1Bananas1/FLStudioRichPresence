#include "process_detector.h"
#include <iostream>
#include <algorithm>
#include <signal.h>

// Platform-specific includes
#ifdef _WIN32
    #include <windows.h>
    #include <tlhelp32.h>
    #include <psapi.h>
#elif __APPLE__
    #include <libproc.h>
    #include <sys/proc_info.h>
    #import <AppKit/AppKit.h>
    #import <Foundation/Foundation.h>
#else // Linux
    #include <dirent.h>
    #include <unistd.h>
    #include <sys/types.h>
    #include <fstream>
    #include <cstdlib>
#endif

std::vector<ProcessInfo> CrossPlatformProcessDetector::GetAllProcesses() {
#ifdef _WIN32
    return GetProcessesWindows();
#elif __APPLE__
    return GetProcessesMacOS();
#else
    return GetProcessesLinux();
#endif
}

std::vector<ProcessInfo> CrossPlatformProcessDetector::GetProcessesByName(const std::string& processName) {
    auto allProcesses = GetAllProcesses();
    std::vector<ProcessInfo> filtered;
    
    for (const auto& process : allProcesses) {
        if (process.name.find(processName) != std::string::npos) {
            filtered.push_back(process);
        }
    }
    
    return filtered;
}

std::string CrossPlatformProcessDetector::GetWindowTitle(int pid) {
#ifdef _WIN32
    return GetWindowTitleWindows(pid);
#elif __APPLE__
    return GetWindowTitleMacOS(pid);
#else
    return GetWindowTitleLinux(pid);
#endif
}

bool CrossPlatformProcessDetector::IsProcessRunning(const std::string& processName) {
    auto processes = GetProcessesByName(processName);
    return !processes.empty();
}

bool CrossPlatformProcessDetector::IsProcessRunning(int pid) {
    if (pid <= 0) return false;
    
#ifdef _WIN32
    HANDLE process = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, pid);
    if (process != nullptr) {
        CloseHandle(process);
        return true;
    }
    return false;
#else
    return kill(pid, 0) == 0;
#endif
}

#ifdef _WIN32
std::vector<ProcessInfo> CrossPlatformProcessDetector::GetProcessesWindows() {
    std::vector<ProcessInfo> processes;
    
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snapshot == INVALID_HANDLE_VALUE) return processes;
    
    PROCESSENTRY32 entry;
    entry.dwSize = sizeof(PROCESSENTRY32);
    
    if (Process32First(snapshot, &entry)) {
        do {
            ProcessInfo info;
            info.pid = entry.th32ProcessID;
            info.name = entry.szExeFile;
            info.windowTitle = GetWindowTitleWindows(info.pid);
            
            // Get executable path
            HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, info.pid);
            if (hProcess) {
                char path[MAX_PATH];
                if (GetModuleFileNameExA(hProcess, nullptr, path, MAX_PATH)) {
                    info.executablePath = path;
                }
                CloseHandle(hProcess);
            }
            
            processes.push_back(info);
        } while (Process32Next(snapshot, &entry));
    }
    
    CloseHandle(snapshot);
    return processes;
}

std::string CrossPlatformProcessDetector::GetWindowTitleWindows(int pid) {
    std::string result;
    
    struct EnumData {
        DWORD pid;
        std::string* title;
    };
    
    EnumData data = { static_cast<DWORD>(pid), &result };
    
    EnumWindows([](HWND hwnd, LPARAM lParam) -> BOOL {
        auto* data = reinterpret_cast<EnumData*>(lParam);
        
        DWORD windowPid;
        GetWindowThreadProcessId(hwnd, &windowPid);
        
        if (windowPid == data->pid && IsWindowVisible(hwnd)) {
            char title[512];
            GetWindowTextA(hwnd, title, sizeof(title));
            if (strlen(title) > 0) {
                *(data->title) = title;
                return FALSE; // Stop enumeration
            }
        }
        return TRUE;
    }, reinterpret_cast<LPARAM>(&data));
    
    return result;
}

#elif __APPLE__
std::vector<ProcessInfo> CrossPlatformProcessDetector::GetProcessesMacOS() {
    std::vector<ProcessInfo> processes;
    
    int numberOfProcesses = proc_listpids(PROC_ALL_PIDS, 0, nullptr, 0);
    std::vector<pid_t> pids(numberOfProcesses);
    
    proc_listpids(PROC_ALL_PIDS, 0, pids.data(), numberOfProcesses * sizeof(pid_t));
    
    for (pid_t pid : pids) {
        if (pid == 0) continue;
        
        ProcessInfo info;
        info.pid = pid;
        
        // Get executable path
        char pathBuffer[PROC_PIDPATHINFO_MAXSIZE];
        if (proc_pidpath(pid, pathBuffer, sizeof(pathBuffer)) > 0) {
            info.executablePath = pathBuffer;
            
            // Extract process name from path
            std::string path(pathBuffer);
            size_t lastSlash = path.find_last_of('/');
            if (lastSlash != std::string::npos) {
                info.name = path.substr(lastSlash + 1);
            } else {
                info.name = path;
            }
        }
        
        info.windowTitle = GetWindowTitleMacOS(pid);
        processes.push_back(info);
    }
    
    return processes;
}

std::string CrossPlatformProcessDetector::GetWindowTitleMacOS(int pid) {
    @autoreleasepool {
        NSArray* runningApps = [[NSWorkspace sharedWorkspace] runningApplications];
        
        for (NSRunningApplication* app in runningApps) {
            if ([app processIdentifier] == pid) {
                NSString* name = [app localizedName];
                if (name) {
                    return std::string([name UTF8String]);
                }
                break;
            }
        }
    }
    
    return "";
}

#else // Linux
std::vector<ProcessInfo> CrossPlatformProcessDetector::GetProcessesLinux() {
    std::vector<ProcessInfo> processes;
    
    DIR* procDir = opendir("/proc");
    if (!procDir) return processes;
    
    struct dirent* entry;
    while ((entry = readdir(procDir)) != nullptr) {
        int pid = atoi(entry->d_name);
        if (pid <= 0) continue;
        
        ProcessInfo info;
        info.pid = pid;
        
        // Read process name from /proc/PID/comm
        std::string commPath = "/proc/" + std::string(entry->d_name) + "/comm";
        std::ifstream commFile(commPath);
        if (commFile.is_open()) {
            std::getline(commFile, info.name);
            commFile.close();
        }
        
        // Read command line for full path
        std::string cmdlinePath = "/proc/" + std::string(entry->d_name) + "/cmdline";
        std::ifstream cmdlineFile(cmdlinePath);
        if (cmdlineFile.is_open()) {
            std::string cmdline;
            std::getline(cmdlineFile, cmdline, '\0');
            
            if (!cmdline.empty()) {
                info.executablePath = cmdline;
                
                // Use full name from cmdline if comm was truncated
                if (info.name.length() >= 15) {
                    size_t lastSlash = cmdline.find_last_of('/');
                    if (lastSlash != std::string::npos) {
                        info.name = cmdline.substr(lastSlash + 1);
                    }
                }
            }
            cmdlineFile.close();
        }
        
        info.windowTitle = GetWindowTitleLinux(pid);
        processes.push_back(info);
    }
    
    closedir(procDir);
    return processes;
}

std::string CrossPlatformProcessDetector::GetWindowTitleLinux(int pid) {
    // Try multiple methods for different window managers
    
    // Method 1: xdotool
    std::string command = "xdotool search --pid " + std::to_string(pid) + " getwindowname %@ 2>/dev/null | head -1";
    FILE* pipe = popen(command.c_str(), "r");
    if (pipe) {
        char buffer[256];
        std::string result;
        if (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
            result = buffer;
            if (!result.empty() && result.back() == '\n') {
                result.pop_back();
            }
        }
        pclose(pipe);
        if (!result.empty()) return result;
    }
    
    // Method 2: wmctrl
    command = "wmctrl -l -p | grep ' " + std::to_string(pid) + " ' | cut -d' ' -f4- 2>/dev/null | head -1";
    pipe = popen(command.c_str(), "r");
    if (pipe) {
        char buffer[256];
        std::string result;
        if (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
            result = buffer;
            if (!result.empty() && result.back() == '\n') {
                result.pop_back();
            }
        }
        pclose(pipe);
        return result;
    }
    
    return "";
}
#endif
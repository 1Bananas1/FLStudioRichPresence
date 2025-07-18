#pragma once

#include <vector>
#include <string>
#include "../include/fl_studio_types.h"

class CrossPlatformProcessDetector {
public:
    static std::vector<ProcessInfo> GetAllProcesses();
    static std::vector<ProcessInfo> GetProcessesByName(const std::string& processName);
    static std::string GetWindowTitle(int pid);
    static bool IsProcessRunning(const std::string& processName);
    static bool IsProcessRunning(int pid);
    
private:
#ifdef _WIN32
    static std::vector<ProcessInfo> GetProcessesWindows();
    static std::string GetWindowTitleWindows(int pid);
#elif __APPLE__
    static std::vector<ProcessInfo> GetProcessesMacOS();
    static std::string GetWindowTitleMacOS(int pid);
#else // Linux
    static std::vector<ProcessInfo> GetProcessesLinux();
    static std::string GetWindowTitleLinux(int pid);
#endif
};
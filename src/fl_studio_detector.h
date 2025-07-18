#pragma once

#include <mutex>
#include <chrono>
#include "../include/fl_studio_types.h"
#include <vector>

class FLStudioDetector {
public:
    FLStudioDetector();
    ~FLStudioDetector() = default;
    
    FLStudioInfo GetCurrentInfo();
    bool IsFLStudioRunning() const;
    
    // Configuration
    void SetUpdateInterval(std::chrono::milliseconds interval);
    
private:
    std::vector<ProcessInfo> FindFLStudioProcesses() const;
    void ParseWindowTitle(const std::string& title, FLStudioInfo& info) const;
    void ExtractProjectName(const std::string& projectPart, FLStudioInfo& info) const;
    void DetectVersion(const std::string& processName, const std::string& title, FLStudioInfo& info) const;
    FLStudioState DetermineState(const FLStudioInfo& info) const;
    
    mutable std::mutex detectionMutex;
    std::chrono::milliseconds updateInterval{2000};
    
    // Cached state
    FLStudioInfo lastInfo;
    std::chrono::steady_clock::time_point lastUpdate;
    
    // FL Studio process names for different platforms
    static const std::vector<std::string> FL_PROCESS_NAMES;
};
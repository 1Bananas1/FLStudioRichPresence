#include "fl_studio_detector.h"
#include "process_detector.h"
#include <algorithm>
#include <regex>
#include <iostream>

const std::vector<std::string> FLStudioDetector::FL_PROCESS_NAMES = {
    "FL64.exe",       // Windows 64-bit
    "FL.exe",         // Windows 32-bit
    "FL Studio 21",   // macOS
    "FL Studio 20",   // macOS
    "FL Studio",      // Generic macOS
    "fl64.exe",       // Linux/Wine (lowercase)
    "fl.exe",         // Linux/Wine (lowercase)
    "wine"            // Linux Wine (check cmdline)
};

FLStudioDetector::FLStudioDetector() {
    lastInfo.sessionStartTime = std::time(nullptr);
    lastUpdate = std::chrono::steady_clock::now();
}

FLStudioInfo FLStudioDetector::GetCurrentInfo() {
    std::lock_guard<std::mutex> lock(detectionMutex);
    
    auto now = std::chrono::steady_clock::now();
    
    // Throttle updates
    if (std::chrono::duration_cast<std::chrono::milliseconds>(now - lastUpdate) < updateInterval) {
        return lastInfo;
    }
    
    FLStudioInfo info;
    auto flProcesses = FindFLStudioProcesses();
    
    if (flProcesses.empty()) {
        info.isRunning = false;
        info.isIdle = true;
        lastInfo = info;
        lastUpdate = now;
        return info;
    }
    
    // Use the first (or most relevant) FL Studio process
    ProcessInfo flProcess = flProcesses[0];
    
    info.isRunning = true;
    info.processId = flProcess.pid;
    info.windowTitle = flProcess.windowTitle;
    info.executablePath = flProcess.executablePath;
    
    // Parse information from window title
    ParseWindowTitle(info.windowTitle, info);
    
    // Detect FL Studio version
    DetectVersion(flProcess.name, info.windowTitle, info);
    
    // Reset session start time if this is a new process
    if (lastInfo.processId != info.processId) {
        info.sessionStartTime = std::time(nullptr);
    } else {
        info.sessionStartTime = lastInfo.sessionStartTime;
    }
    
    info.lastActivity = std::time(nullptr);
    
    lastInfo = info;
    lastUpdate = now;
    
    return info;
}

bool FLStudioDetector::IsFLStudioRunning() const {
    return !FindFLStudioProcesses().empty();
}

void FLStudioDetector::SetUpdateInterval(std::chrono::milliseconds interval) {
    std::lock_guard<std::mutex> lock(detectionMutex);
    updateInterval = interval;
}

std::vector<ProcessInfo> FLStudioDetector::FindFLStudioProcesses() const {
    std::vector<ProcessInfo> flProcesses;
    auto allProcesses = CrossPlatformProcessDetector::GetAllProcesses();
    
    for (const auto& process : allProcesses) {
        // Check if process name matches any FL Studio variants
        for (const auto& flName : FL_PROCESS_NAMES) {
            if (process.name.find(flName) != std::string::npos) {
                // Additional validation for Wine processes
                if (process.name == "wine") {
                    if (process.executablePath.find("FL") != std::string::npos ||
                        process.executablePath.find("fl") != std::string::npos) {
                        flProcesses.push_back(process);
                    }
                } else {
                    flProcesses.push_back(process);
                }
                break;
            }
        }
        
        // Also check window titles for macOS
        if (process.windowTitle.find("FL Studio") != std::string::npos) {
            flProcesses.push_back(process);
        }
    }
    
    return flProcesses;
}

void FLStudioDetector::ParseWindowTitle(const std::string& title, FLStudioInfo& info) const {
    if (title.empty()) return;
    
    // Different title formats:
    // Windows: "FL Studio 21 - MyProject.flp"
    // macOS: "MyProject.flp — FL Studio 21"
    // Some versions: "FL Studio 21 - MyProject.flp *" (unsaved)
    
    // Try Windows format first (most common)
    size_t dashPos = title.find(" - ");
    if (dashPos != std::string::npos) {
        std::string projectPart = title.substr(dashPos + 3);
        ExtractProjectName(projectPart, info);
        return;
    }
    
    // Try macOS format with em dash
    size_t emDashPos = title.find(" — ");
    if (emDashPos != std::string::npos) {
        std::string projectPart = title.substr(0, emDashPos);
        ExtractProjectName(projectPart, info);
        return;
    }
    
    // Try to extract project name from various other formats
    std::regex projectRegex(R"(([^-—]+\.flp)\s*\*?)");
    std::smatch match;
    if (std::regex_search(title, match, projectRegex)) {
        ExtractProjectName(match[1].str(), info);
    }
}

void FLStudioDetector::ExtractProjectName(const std::string& projectPart, FLStudioInfo& info) const {
    std::string cleaned = projectPart;
    
    // Remove .flp extension
    if (cleaned.length() > 4 && cleaned.substr(cleaned.length() - 4) == ".flp") {
        cleaned = cleaned.substr(0, cleaned.length() - 4);
    }
    
    // Remove unsaved indicator (*)
    while (!cleaned.empty() && (cleaned.back() == '*' || cleaned.back() == ' ')) {
        cleaned.pop_back();
    }
    
    // Trim whitespace
    cleaned.erase(0, cleaned.find_first_not_of(" \t\r\n"));
    cleaned.erase(cleaned.find_last_not_of(" \t\r\n") + 1);
    
    // Check for valid project name
    if (!cleaned.empty() && 
        cleaned != "Untitled" && 
        cleaned != "Untitled.flp" &&
        cleaned.length() > 0) {
        info.projectName = cleaned;
        info.hasUnsavedChanges = projectPart.find('*') != std::string::npos;
    }
}

void FLStudioDetector::DetectVersion(const std::string& processName, const std::string& title, FLStudioInfo& info) const {
    // Extract version from window title first (most reliable)
    if (title.find("FL Studio 21") != std::string::npos) {
        info.version = "FL Studio 21";
    } else if (title.find("FL Studio 20") != std::string::npos) {
        info.version = "FL Studio 20";
    } else if (title.find("FL Studio 12") != std::string::npos) {
        info.version = "FL Studio 12";
    } else if (title.find("FL Studio") != std::string::npos) {
        info.version = "FL Studio";
    }
    // Fall back to process name analysis
    else if (processName.find("FL64") != std::string::npos) {
        info.version = "FL Studio (64-bit)";
    } else if (processName.find("FL.exe") != std::string::npos) {
        info.version = "FL Studio (32-bit)";
    } else {
        info.version = "FL Studio";
    }
}

FLStudioState FLStudioDetector::DetermineState(const FLStudioInfo& info) const {
    if (!info.isRunning) {
        return FLStudioState::NotRunning;
    }
    
    if (info.isRecording) {
        return FLStudioState::Recording;
    }
    
    if (info.isPlaying) {
        return FLStudioState::Playing;
    }
    
    if (info.isPaused) {
        return FLStudioState::Paused;
    }
    
    if (!info.projectName.empty()) {
        return FLStudioState::Composing;
    }
    
    return FLStudioState::Idle;
}
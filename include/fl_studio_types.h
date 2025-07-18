#pragma once

#include <string>
#include <chrono>
#include <ctime>

struct ProcessInfo {
    int pid = 0;
    std::string name;
    std::string windowTitle;
    std::string executablePath;
    bool isVisible = false;
};

struct FLStudioInfo {
    // Basic info
    std::string projectName;
    std::string projectPath;
    std::string version = "FL Studio";
    std::string windowTitle;
    std::string executablePath;  // Add this missing member
    
    // State
    bool isRunning = false;
    bool isPlaying = false;
    bool isRecording = false;
    bool isPaused = false;
    bool isIdle = true;
    bool hasUnsavedChanges = false;
    
    // Process info
    int processId = 0;
    
    // Project details (harder to detect cross-platform)
    int bpm = 0;
    int currentPattern = 0;
    
    // Timing
    std::time_t sessionStartTime = 0;
    std::time_t lastActivity = 0;
    
    // Comparison for change detection
    bool operator!=(const FLStudioInfo& other) const {
        return projectName != other.projectName ||
               isPlaying != other.isPlaying ||
               isRecording != other.isRecording ||
               isPaused != other.isPaused ||
               isRunning != other.isRunning ||
               hasUnsavedChanges != other.hasUnsavedChanges;
    }
};

enum class FLStudioState {
    NotRunning,
    Idle,
    Composing,
    Playing,
    Recording,
    Paused
};

// Discord asset names
namespace DiscordAssets {
    constexpr const char* FL_STUDIO_LOGO = "fl_studio_logo";
    constexpr const char* PLAYING = "playing";
    constexpr const char* RECORDING = "recording";
    constexpr const char* COMPOSING = "composing";
    constexpr const char* PAUSED = "paused";
    constexpr const char* IDLE = "idle";
}
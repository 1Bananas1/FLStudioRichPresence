#pragma once

#include <string>
#include <vector>
#include <chrono>

struct AppConfig {
    // Discord settings
    std::string applicationId = "1395851731312836760";
    bool enableRichPresence = true;
    
    // Privacy settings
    bool showProjectName = true;
    bool showProjectPath = false;
    bool showBPM = true;
    bool showPlaybackState = true;
    bool showUnsavedChanges = true;
    std::vector<std::string> hiddenProjects;
    
    // Update settings
    std::chrono::milliseconds updateInterval{3000};
    std::chrono::seconds presenceTimeout{30};
    
    // Advanced features
    bool enableAdvancedDetection = false;
    bool enableAudioDetection = false;
    bool enableCustomButtons = true;
    
    // System settings
    bool minimizeToTray = false;
    bool startWithSystem = false;
    bool showNotifications = true;
    bool enableLogging = true;
    
    // Custom messages
    std::string customIdleMessage;
    std::string customComposingMessage;
    std::string customPlayingMessage;
    std::string customRecordingMessage;
    
    // File operations
    static AppConfig Load(const std::string& configPath = "");
    bool Save(const std::string& configPath = "") const;
    
    // Validation
    bool IsValid() const;
    void SetDefaults();
    
private:
    static std::string GetDefaultConfigPath();
    static std::string GetConfigDirectory();
};
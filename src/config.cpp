#include "config.h"
#include <fstream>
#include <iostream>
#include <filesystem>

#ifdef _WIN32
    #include <shlobj.h>
    #include <windows.h>
#elif __APPLE__
    #include <unistd.h>
    #include <pwd.h>
#else
    #include <unistd.h>
    #include <pwd.h>
#endif

// Simple JSON-like parser (you might want to use a real JSON library)
#include <sstream>
#include <map>

AppConfig AppConfig::Load(const std::string& configPath) {
    AppConfig config;
    std::string path = configPath.empty() ? GetDefaultConfigPath() : configPath;
    
    try {
        if (std::filesystem::exists(path)) {
            std::ifstream file(path);
            if (file.is_open()) {
                std::string line;
                while (std::getline(file, line)) {
                    // Simple key=value parser
                    size_t equalPos = line.find('=');
                    if (equalPos != std::string::npos) {
                        std::string key = line.substr(0, equalPos);
                        std::string value = line.substr(equalPos + 1);
                        
                        // Remove quotes if present
                        if (value.front() == '"' && value.back() == '"') {
                            value = value.substr(1, value.length() - 2);
                        }
                        
                        // Parse configuration values
                        if (key == "applicationId") config.applicationId = value;
                        else if (key == "enableRichPresence") config.enableRichPresence = (value == "true");
                        else if (key == "showProjectName") config.showProjectName = (value == "true");
                        else if (key == "showBPM") config.showBPM = (value == "true");
                        else if (key == "updateInterval") config.updateInterval = std::chrono::milliseconds(std::stoi(value));
                        // Add more config parsing as needed
                    }
                }
                file.close();
                
                std::cout << "Configuration loaded from: " << path << std::endl;
            }
        } else {
            std::cout << "Config file not found, using defaults: " << path << std::endl;
            config.SetDefaults();
            config.Save(path); // Create default config file
        }
    } catch (const std::exception& e) {
        std::cerr << "Failed to load config: " << e.what() << std::endl;
        config.SetDefaults();
    }
    
    return config;
}

bool AppConfig::Save(const std::string& configPath) const {
    std::string path = configPath.empty() ? GetDefaultConfigPath() : configPath;
    
    try {
        // Create directory if it doesn't exist
        std::string dir = GetConfigDirectory();
        if (!dir.empty()) {
            std::filesystem::create_directories(dir);
        }
        
        std::ofstream file(path);
        if (!file.is_open()) {
            std::cerr << "Failed to open config file for writing: " << path << std::endl;
            return false;
        }
        
        // Write configuration in simple key=value format
        file << "# FL Studio Discord Rich Presence Configuration\n";
        file << "applicationId=\"" << applicationId << "\"\n";
        file << "enableRichPresence=" << (enableRichPresence ? "true" : "false") << "\n";
        file << "showProjectName=" << (showProjectName ? "true" : "false") << "\n";
        file << "showBPM=" << (showBPM ? "true" : "false") << "\n";
        file << "updateInterval=" << updateInterval.count() << "\n";
        file << "enableLogging=" << (enableLogging ? "true" : "false") << "\n";
        // Add more config writing as needed
        
        file.close();
        std::cout << "Configuration saved to: " << path << std::endl;
        return true;
        
    } catch (const std::exception& e) {
        std::cerr << "Failed to save config: " << e.what() << std::endl;
        return false;
    }
}

bool AppConfig::IsValid() const {
    return !applicationId.empty() && 
           applicationId != "YOUR_DISCORD_APP_ID_HERE" &&
           updateInterval.count() > 0;
}

void AppConfig::SetDefaults() {
    applicationId = "YOUR_DISCORD_APP_ID_HERE";
    enableRichPresence = true;
    showProjectName = true;
    showBPM = true;
    updateInterval = std::chrono::milliseconds(3000);
    enableLogging = true;
}

std::string AppConfig::GetDefaultConfigPath() {
    std::string dir = GetConfigDirectory();
    return dir.empty() ? "config.txt" : dir + "/config.txt";
}

std::string AppConfig::GetConfigDirectory() {
#ifdef _WIN32
    char* appDataPath;
    if (SHGetKnownFolderPath(FOLDERID_RoamingAppData, 0, nullptr, (PWSTR*)&appDataPath) == S_OK) {
        std::string path = appDataPath;
        CoTaskMemFree(appDataPath);
        return path + "\\FLStudioDiscordRPC";
    }
#elif __APPLE__
    const char* homeDir = getenv("HOME");
    if (!homeDir) {
        struct passwd* pw = getpwuid(getuid());
        homeDir = pw->pw_dir;
    }
    if (homeDir) {
        return std::string(homeDir) + "/Library/Application Support/FLStudioDiscordRPC";
    }
#else // Linux
    const char* homeDir = getenv("HOME");
    if (!homeDir) {
        struct passwd* pw = getpwuid(getuid());
        homeDir = pw->pw_dir;
    }
    if (homeDir) {
        // Use XDG config directory if available
        const char* xdgConfig = getenv("XDG_CONFIG_HOME");
        if (xdgConfig) {
            return std::string(xdgConfig) + "/flstudio-discord-rpc";
        } else {
            return std::string(homeDir) + "/.config/flstudio-discord-rpc";
        }
    }
#endif
    
    return ""; // Fallback to current directory
}
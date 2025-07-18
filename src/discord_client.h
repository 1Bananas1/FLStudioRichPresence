#pragma once

#include <memory>
#include <functional>
#include <string>
#include <atomic>
#include <thread>
#include <chrono>

// Include Discord SDK header with relative path
#include "../discord_social_sdk/include/discordpp.h"

#include "../include/fl_studio_types.h"

// Forward declare FL Studio detector to avoid circular dependency
class FLStudioDetector;

class DiscordClient {
public:
    using UpdateCallback = std::function<void(bool success, const std::string& error)>;
    
    explicit DiscordClient(const std::string& applicationId);
    ~DiscordClient();
    
    // Core functionality
    bool Initialize();
    void Shutdown();
    void RunCallbacks(); // Must be called regularly
    
    // Rich Presence
    void UpdateRichPresence(const FLStudioInfo& info, UpdateCallback callback = nullptr);
    void ClearPresence();
    
    // Status
    bool IsConnected() const;
    bool IsInitialized() const;
    
private:
    class Impl;
    std::unique_ptr<Impl> pImpl;
};

// Main application class
class FLStudioDiscordApp {
public:
    explicit FLStudioDiscordApp(const std::string& applicationId);
    ~FLStudioDiscordApp();
    
    bool Initialize();
    void Run();
    void Stop();
    
    // Configuration
    void SetUpdateInterval(std::chrono::milliseconds interval);
    void SetShowProjectName(bool show);
    void SetShowBPM(bool show);
    
private:
    void UpdateLoop();
    std::string BuildDetailsString(const FLStudioInfo& info) const;
    std::string BuildStateString(const FLStudioInfo& info) const;
    
    // Use Pimpl pattern to avoid incomplete type issues
    class AppImpl;
    std::unique_ptr<AppImpl> pImpl;
};
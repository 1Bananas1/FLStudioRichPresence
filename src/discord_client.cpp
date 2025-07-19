// discord_client.cpp - Simple Discord RPC approach
#include "discord_client.h"
#include "fl_studio_detector.h"
#include <iostream>
#include <thread>
#include <chrono>
#include <ctime>

// For now, let's implement a simple mock that works without the complex Partner SDK
class DiscordClient::Impl {
public:
    std::string applicationId;
    bool initialized = false;
    bool connected = false;
    bool isReady = false;
    
    // Simple state tracking
    FLStudioInfo lastInfo;
    std::chrono::steady_clock::time_point startTime;
    
    explicit Impl(const std::string& appId) : applicationId(appId) {
        startTime = std::chrono::steady_clock::now();
    }
    
    bool Initialize() {
        std::cout << "Initializing Simple Discord RPC with App ID: " << applicationId << std::endl;
        
        // For now, simulate successful initialization
        // TODO: Implement actual Discord RPC connection
        initialized = true;
        connected = true;
        isReady = true;
        
        std::cout << "Discord RPC simulated successfully (Partner SDK requires full OAuth)" << std::endl;
        std::cout << "Note: For actual Discord integration, you'll need to:" << std::endl;
        std::cout << "1. Set up OAuth2 in your Discord app" << std::endl;
        std::cout << "2. Add redirect URI: http://127.0.0.1/callback" << std::endl;
        std::cout << "3. Use proper authentication flow" << std::endl;
        
        return true;
    }
    
    void UpdateActivity(const FLStudioInfo& info, DiscordClient::UpdateCallback callback) {
        if (!initialized) {
            if (callback) callback(false, "Discord not initialized");
            return;
        }
        
        // Log what we would send to Discord
        std::string details = BuildDetails(info);
        std::string state = BuildState(info);
        
        auto now = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::seconds>(now - startTime).count();
        
        std::cout << "\n--- Discord Rich Presence Update ---" << std::endl;
        std::cout << "Details: " << details << std::endl;
        std::cout << "State: " << state << std::endl;
        std::cout << "Large Image: fl_studio_logo" << std::endl;
        
        if (info.isRecording) {
            std::cout << "Small Image: recording (Recording)" << std::endl;
        } else if (info.isPlaying) {
            std::cout << "Small Image: playing (Playing)" << std::endl;
        } else if (!info.projectName.empty()) {
            std::cout << "Small Image: composing (Composing)" << std::endl;
        } else {
            std::cout << "Small Image: idle (Idle)" << std::endl;
        }
        
        std::cout << "Session Duration: " << duration << " seconds" << std::endl;
        std::cout << "-----------------------------------\n" << std::endl;
        
        lastInfo = info;
        
        if (callback) {
            callback(true, "Simulated update successful");
        }
    }
    
    void ClearActivity() {
        if (!initialized) return;
        
        std::cout << "Discord presence cleared (simulated)" << std::endl;
    }
    
    void Shutdown() {
        if (initialized) {
            ClearActivity();
            initialized = false;
            connected = false;
            isReady = false;
            std::cout << "Discord RPC shut down (simulated)" << std::endl;
        }
    }
    
    void RunCallbacks() {
        // In a real implementation, this would process Discord SDK callbacks
        // For now, just a no-op
    }
    
private:
    std::string BuildDetails(const FLStudioInfo& info) {
        if (!info.isRunning) {
            return "FL Studio";
        }
        
        if (!info.projectName.empty()) {
            return "Working on " + info.projectName;
        }
        
        if (info.isRecording) {
            return "Recording";
        } else if (info.isPlaying) {
            return "Playing music";
        } else {
            return "Composing music";
        }
    }
    
    std::string BuildState(const FLStudioInfo& info) {
        std::string state = info.version;
        
        if (info.bpm > 0) {
            state += " • " + std::to_string(info.bpm) + " BPM";
        }
        
        if (info.hasUnsavedChanges) {
            state += " • Unsaved";
        }
        
        return state;
    }
};

// DiscordClient implementation
DiscordClient::DiscordClient(const std::string& applicationId) 
    : pImpl(std::make_unique<Impl>(applicationId)) {}

DiscordClient::~DiscordClient() = default;

bool DiscordClient::Initialize() {
    return pImpl->Initialize();
}

void DiscordClient::Shutdown() {
    pImpl->Shutdown();
}

void DiscordClient::UpdateRichPresence(const FLStudioInfo& info, UpdateCallback callback) {
    pImpl->UpdateActivity(info, callback);
}

void DiscordClient::ClearPresence() {
    pImpl->ClearActivity();
}

bool DiscordClient::IsConnected() const {
    return pImpl->connected;
}

bool DiscordClient::IsInitialized() const {
    return pImpl->initialized;
}

void DiscordClient::RunCallbacks() {
    pImpl->RunCallbacks();
}

// FLStudioDiscordApp implementation remains the same as before...
class FLStudioDiscordApp::AppImpl {
public:
    std::unique_ptr<DiscordClient> discord;
    std::unique_ptr<FLStudioDetector> detector;
    
    std::atomic<bool> running{false};
    std::thread updateThread;
    
    // Configuration
    std::chrono::milliseconds updateInterval{3000};
    bool showProjectName = true;
    bool showBPM = true;
    
    // State tracking
    FLStudioInfo lastInfo;
    std::chrono::steady_clock::time_point lastUpdate;
    
    explicit AppImpl(const std::string& applicationId)
        : discord(std::make_unique<DiscordClient>(applicationId))
        , detector(std::make_unique<FLStudioDetector>()) {
    }
};

FLStudioDiscordApp::FLStudioDiscordApp(const std::string& applicationId)
    : pImpl(std::make_unique<AppImpl>(applicationId)) {
}

FLStudioDiscordApp::~FLStudioDiscordApp() {
    Stop();
}

bool FLStudioDiscordApp::Initialize() {
    std::cout << "Initializing FL Studio Discord Rich Presence..." << std::endl;
    
    if (!pImpl->discord->Initialize()) {
        std::cerr << "Failed to initialize Discord client" << std::endl;
        return false;
    }
    
    pImpl->detector->SetUpdateInterval(pImpl->updateInterval);
    
    std::cout << "FL Studio Discord Rich Presence initialized successfully" << std::endl;
    return true;
}

void FLStudioDiscordApp::Run() {
    if (pImpl->running.load()) {
        std::cout << "Application is already running" << std::endl;
        return;
    }
    
    pImpl->running.store(true);
    pImpl->updateThread = std::thread(&FLStudioDiscordApp::UpdateLoop, this);
    
    std::cout << "FL Studio Discord Rich Presence is running..." << std::endl;
    std::cout << "Monitoring for FL Studio processes..." << std::endl;
    std::cout << "Open FL Studio to see rich presence updates below:" << std::endl;
    
    // Keep main thread alive
    while (pImpl->running.load()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    
    if (pImpl->updateThread.joinable()) {
        pImpl->updateThread.join();
    }
}

void FLStudioDiscordApp::Stop() {
    if (!pImpl->running.load()) return;
    
    std::cout << "Stopping FL Studio Discord Rich Presence..." << std::endl;
    pImpl->running.store(false);
    
    if (pImpl->updateThread.joinable()) {
        pImpl->updateThread.join();
    }
    
    pImpl->discord->ClearPresence();
    pImpl->discord->Shutdown();
    
    std::cout << "FL Studio Discord Rich Presence stopped" << std::endl;
}

void FLStudioDiscordApp::SetUpdateInterval(std::chrono::milliseconds interval) {
    pImpl->updateInterval = interval;
    if (pImpl->detector) {
        pImpl->detector->SetUpdateInterval(interval);
    }
}

void FLStudioDiscordApp::SetShowProjectName(bool show) {
    pImpl->showProjectName = show;
}

void FLStudioDiscordApp::SetShowBPM(bool show) {
    pImpl->showBPM = show;
}

void FLStudioDiscordApp::UpdateLoop() {
    auto lastPresenceUpdate = std::chrono::steady_clock::now();
    
    while (pImpl->running.load()) {
        try {
            // Run Discord callbacks (no-op in simulation)
            pImpl->discord->RunCallbacks();
            
            FLStudioInfo currentInfo = pImpl->detector->GetCurrentInfo();
            auto now = std::chrono::steady_clock::now();
            
            // Check if we should update Discord presence
            bool shouldUpdate = (
                currentInfo != pImpl->lastInfo ||
                std::chrono::duration_cast<std::chrono::seconds>(now - lastPresenceUpdate).count() > 30
            );
            
            if (shouldUpdate) {
                pImpl->discord->UpdateRichPresence(currentInfo, [&](bool success, const std::string& error) {
                    if (!success) {
                        std::cerr << "Failed to update Discord presence: " << error << std::endl;
                    }
                });
                
                pImpl->lastInfo = currentInfo;
                lastPresenceUpdate = now;
            }
            
        } catch (const std::exception& e) {
            std::cerr << "Error in update loop: " << e.what() << std::endl;
        }
        
        // Sleep for the configured interval
        std::this_thread::sleep_for(pImpl->updateInterval);
    }
}

std::string FLStudioDiscordApp::BuildDetailsString(const FLStudioInfo& info) const {
    if (!info.isRunning) {
        return "FL Studio not running";
    }
    
    if (pImpl->showProjectName && !info.projectName.empty()) {
        return "Working on " + info.projectName;
    }
    
    if (info.isRecording) {
        return "Recording";
    } else if (info.isPlaying) {
        return "Playing";
    } else {
        return "Composing";
    }
}

std::string FLStudioDiscordApp::BuildStateString(const FLStudioInfo& info) const {
    std::string state = info.version;
    
    if (pImpl->showBPM && info.bpm > 0) {
        state += " • " + std::to_string(info.bpm) + " BPM";
    }
    
    if (info.hasUnsavedChanges) {
        state += " • Unsaved changes";
    }
    
    return state;
}
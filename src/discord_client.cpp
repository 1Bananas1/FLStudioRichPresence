// CRITICAL: Define this ONLY in discord_client.cpp (exactly one file)
#define DISCORDPP_IMPLEMENTATION
#include "../discord_social_sdk/include/discordpp.h"

#include "discord_client.h"
#include "fl_studio_detector.h"
#include <iostream>
#include <thread>

class DiscordClient::Impl {
public:
    std::string applicationId;
    bool initialized = false;
    bool connected = false;
    bool isReady = false;
    
    // Discord SDK client
    std::unique_ptr<discordpp::Client> client;
    
    explicit Impl(const std::string& appId) : applicationId(appId) {}
    
    bool Initialize() {
        std::cout << "Initializing Discord SDK with App ID: " << applicationId << std::endl;
        
        try {
            // Create Discord client
            client = std::make_unique<discordpp::Client>();
            
            // Set up logging callback
            try {
                client->AddLogCallback([](const std::string& message, discordpp::LoggingSeverity severity) {
                    std::cout << "[Discord SDK] " << message << std::endl;
                }, discordpp::LoggingSeverity::Info);
            } catch (...) {
                std::cout << "Log callback setup failed, continuing..." << std::endl;
            }
            
            // Set up status change callback
            client->SetStatusChangedCallback([this](discordpp::Client::Status status, 
                                                   discordpp::Client::Error error, 
                                                   std::uint32_t errorDetail) {
                HandleStatusChange(status, error, errorDetail);
            });
            
            // Convert string application ID to uint64_t
            uint64_t appId = std::stoull(applicationId);
            
            // For Rich Presence only, we can use provisional accounts
            // Using OIDC for external auth type to get a provisional token
            client->GetProvisionalToken(appId,
                                       discordpp::AuthenticationExternalAuthType::OIDC,
                                       "", // empty external auth token for provisional account
                                       [this](discordpp::ClientResult result,
                                              const std::string& accessToken,
                                              const std::string& refreshToken,
                                              discordpp::AuthorizationTokenType tokenType,
                                              std::uint32_t expiresIn,
                                              const std::string& scopes) {
                HandleProvisionalToken(result, accessToken, tokenType);
            });
            
            initialized = true;
            std::cout << "Discord SDK initialization started..." << std::endl;
            return true;
            
        } catch (const std::exception& e) {
            std::cerr << "Exception during Discord SDK initialization: " << e.what() << std::endl;
            return false;
        }
    }
    
    void HandleProvisionalToken(discordpp::ClientResult result,
                              const std::string& accessToken,
                              discordpp::AuthorizationTokenType tokenType) {
        if (result.Successful()) {
            std::cout << "Provisional token received, updating client..." << std::endl;
            
            client->UpdateToken(tokenType, accessToken, [this](discordpp::ClientResult result) {
                if (result.Successful()) {
                    std::cout << "Token updated, connecting to Discord..." << std::endl;
                    client->Connect();
                } else {
                    std::cerr << "Failed to update token: " << result.Error() << std::endl;
                }
            });
        } else {
            std::cerr << "Failed to get provisional token: " << result.Error() << std::endl;
        }
    }
    
    void HandleStatusChange(discordpp::Client::Status status, 
                          discordpp::Client::Error error, 
                          std::uint32_t errorDetail) {
        std::cout << "Discord SDK status changed to: " << static_cast<int>(status) << std::endl;
        
        switch (status) {
            case discordpp::Client::Status::Ready:
                std::cout << "Discord SDK is ready!" << std::endl;
                isReady = true;
                connected = true;
                break;
            case discordpp::Client::Status::Connecting:
                std::cout << "Discord SDK connecting..." << std::endl;
                break;
            case discordpp::Client::Status::Connected:
                std::cout << "Discord SDK connected, waiting for ready..." << std::endl;
                break;
            case discordpp::Client::Status::Disconnected:
                std::cout << "Discord SDK disconnected" << std::endl;
                connected = false;
                isReady = false;
                break;
            case discordpp::Client::Status::Reconnecting:
                std::cout << "Discord SDK reconnecting..." << std::endl;
                break;
            default:
                if (error != discordpp::Client::Error::None) {
                    std::cerr << "Discord SDK error: " << static_cast<int>(error) 
                              << " (detail: " << errorDetail << ")" << std::endl;
                }
                break;
        }
    }
    
    void UpdateActivity(const FLStudioInfo& info, DiscordClient::UpdateCallback callback) {
        if (!initialized || !client || !isReady) {
            if (callback) callback(false, "Discord not ready");
            return;
        }
        
        try {
            // Create Discord activity
            discordpp::Activity activity;
            
            // Set basic details
            std::string details = BuildDetails(info);
            std::string state = BuildState(info);
            
            activity.SetDetails(details);
            activity.SetState(state);
            activity.SetType(discordpp::ActivityTypes::Playing); // Fixed: ActivityTypes (plural) and Playing
            
            // Set up assets
            discordpp::ActivityAssets assets;
            assets.SetLargeImage(DiscordAssets::FL_STUDIO_LOGO);
            assets.SetLargeText("FL Studio");
            
            // Dynamic small icon based on state
            if (info.isRecording) {
                assets.SetSmallImage(DiscordAssets::RECORDING);
                assets.SetSmallText("Recording");
            } else if (info.isPlaying) {
                assets.SetSmallImage(DiscordAssets::PLAYING);
                assets.SetSmallText("Playing");
            } else if (!info.projectName.empty()) {
                assets.SetSmallImage(DiscordAssets::COMPOSING);
                assets.SetSmallText("Composing");
            } else {
                assets.SetSmallImage(DiscordAssets::IDLE);
                assets.SetSmallText("Idle");
            }
            
            activity.SetAssets(assets);
            
            // Set timestamps
            if (info.sessionStartTime > 0) {
                discordpp::ActivityTimestamps timestamps;
                timestamps.SetStart(info.sessionStartTime * 1000); // Convert to milliseconds
                activity.SetTimestamps(timestamps);
            }
            
            // Optional: Add custom buttons
            if (!info.projectName.empty()) {
                discordpp::ActivityButton button;
                button.SetLabel("Get FL Studio");
                button.SetUrl("https://www.image-line.com/");
                activity.AddButton(button);
            }
            
            // Update Discord presence
            client->UpdateRichPresence(activity, [callback](discordpp::ClientResult result) {
                if (callback) {
                    if (result.Successful()) {
                        callback(true, "");
                    } else {
                        callback(false, "Failed to update Discord activity: " + result.Error());
                    }
                }
            });
            
        } catch (const std::exception& e) {
            std::cerr << "Exception during activity update: " << e.what() << std::endl;
            if (callback) callback(false, "Exception: " + std::string(e.what()));
        }
    }
    
    void ClearActivity() {
        if (!initialized || !client || !isReady) return;
        
        try {
            // Clear by setting empty activity
            discordpp::Activity emptyActivity;
            client->UpdateRichPresence(emptyActivity, [](discordpp::ClientResult result) {
                if (result.Successful()) {
                    std::cout << "Discord presence cleared" << std::endl;
                } else {
                    std::cerr << "Failed to clear Discord presence: " << result.Error() << std::endl;
                }
            });
        } catch (const std::exception& e) {
            std::cerr << "Exception during clear activity: " << e.what() << std::endl;
        }
    }
    
    void Shutdown() {
        if (initialized && client) {
            try {
                ClearActivity();
                // Note: Discord SDK doesn't have explicit shutdown, just destroy the client
                client.reset();
                initialized = false;
                connected = false;
                isReady = false;
                std::cout << "Discord SDK shut down successfully" << std::endl;
            } catch (const std::exception& e) {
                std::cerr << "Exception during shutdown: " << e.what() << std::endl;
            }
        }
    }
    
    void RunCallbacks() {
        // This must be called regularly to process Discord SDK callbacks
        if (initialized) {
            discordpp::RunCallbacks();
        }
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

// Add method to run callbacks
void DiscordClient::RunCallbacks() {
    pImpl->RunCallbacks();
}

// FLStudioDiscordApp implementation
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
            // CRITICAL: Run Discord SDK callbacks
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
                    if (success) {
                        std::cout << "Updated presence: " << currentInfo.projectName 
                                  << " (" << currentInfo.version << ")" << std::endl;
                    } else {
                        std::cerr << "Failed to update Discord presence: " << error << std::endl;
                    }
                });
                
                pImpl->lastInfo = currentInfo;
                lastPresenceUpdate = now;
            }
            
        } catch (const std::exception& e) {
            std::cerr << "Error in update loop: " << e.what() << std::endl;
        }
        
        // Sleep for a shorter interval since we need to run callbacks regularly
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
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
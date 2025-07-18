#include <iostream>
#include <signal.h>
#include <memory>

#include "discord_client.h"
#include "config.h"

// Global app instance for signal handling
std::unique_ptr<FLStudioDiscordApp> g_app = nullptr;

void SignalHandler(int signal) {
    std::cout << "\nReceived signal " << signal << ", shutting down gracefully..." << std::endl;
    if (g_app) {
        g_app->Stop();
    }
    exit(0);
}

int main(int argc, char* argv[]) {
    std::cout << "FL Studio Discord Rich Presence v1.0.0" << std::endl;
    std::cout << "Cross-platform FL Studio activity tracking for Discord" << std::endl;
    std::cout << "==========================================================" << std::endl;
    
    // Set up signal handling for graceful shutdown
    signal(SIGINT, SignalHandler);   // Ctrl+C
    signal(SIGTERM, SignalHandler);  // Termination request
#ifdef _WIN32
    signal(SIGBREAK, SignalHandler); // Ctrl+Break on Windows
#endif
    
    try {
        // Load configuration
        std::cout << "Loading configuration..." << std::endl;
        auto config = AppConfig::Load();
        
        if (!config.IsValid()) {
            std::cerr << "ERROR: Invalid configuration!" << std::endl;
            std::cerr << "Please set your Discord Application ID in the config file." << std::endl;
            std::cerr << "1. Go to https://discord.com/developers/applications" << std::endl;
            std::cerr << "2. Create a new application" << std::endl;
            std::cerr << "3. Copy the Application ID" << std::endl;
            std::cerr << "4. Update the config file with your Application ID" << std::endl;
            return 1;
        }
        
        // Create and initialize the application
        g_app = std::make_unique<FLStudioDiscordApp>(config.applicationId);
        
        // Configure the app
        g_app->SetUpdateInterval(config.updateInterval);
        g_app->SetShowProjectName(config.showProjectName);
        g_app->SetShowBPM(config.showBPM);
        
        if (!g_app->Initialize()) {
            std::cerr << "ERROR: Failed to initialize FL Studio Discord Rich Presence" << std::endl;
            std::cerr << "Make sure Discord is running and try again." << std::endl;
            return 1;
        }
        
        std::cout << "Initialization successful!" << std::endl;
        std::cout << "Press Ctrl+C to exit" << std::endl;
        std::cout << "------------------------------------------" << std::endl;
        
        // Run the application (this blocks until shutdown)
        g_app->Run();
        
    } catch (const std::exception& e) {
        std::cerr << "FATAL ERROR: " << e.what() << std::endl;
        return 1;
    }
    
    std::cout << "FL Studio Discord Rich Presence stopped cleanly." << std::endl;
    return 0;
}
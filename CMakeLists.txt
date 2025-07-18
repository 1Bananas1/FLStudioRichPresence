cmake_minimum_required(VERSION 3.20)
project(FLStudioDiscordRPC VERSION 1.0.0 LANGUAGES CXX)

# Add this at the very top to test
message(STATUS "=== CMAKE DEBUG START ===")
message(STATUS "CMake source dir: ${CMAKE_SOURCE_DIR}")
message(STATUS "CMake binary dir: ${CMAKE_BINARY_DIR}")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Platform detection
if(WIN32)
    set(PLATFORM_LIBS user32 psapi)
    message(STATUS "Platform: Windows")
elseif(APPLE)
    find_library(CORE_FOUNDATION CoreFoundation)
    find_library(APPLICATION_SERVICES ApplicationServices)
    set(PLATFORM_LIBS ${CORE_FOUNDATION} ${APPLICATION_SERVICES})
    set_source_files_properties(src/process_detector.cpp PROPERTIES COMPILE_FLAGS "-x objective-c++")
    message(STATUS "Platform: macOS")
else()
    set(PLATFORM_LIBS pthread asound pulse)  # Added asound and pulse for Discord SDK
    message(STATUS "Platform: Linux")
endif()

# Discord Social SDK - use absolute path
set(DISCORD_SDK_PATH "${CMAKE_SOURCE_DIR}/discord_social_sdk")

# Debug: Print the path being used
message(STATUS "=== DISCORD SDK DETECTION ===")
message(STATUS "Looking for Discord SDK at: ${DISCORD_SDK_PATH}")

# Check if the directory exists first
if(EXISTS "${DISCORD_SDK_PATH}")
    message(STATUS "✓ Discord SDK directory exists")
    
    # Check for headers
    if(EXISTS "${DISCORD_SDK_PATH}/include/discordpp.h")
        message(STATUS "✓ Found discordpp.h")
    else()
        message(STATUS "❌ discordpp.h not found")
    endif()
    
    # Check for libraries
    if(EXISTS "${DISCORD_SDK_PATH}/lib")
        message(STATUS "✓ lib directory exists")
        
        # List what's in the lib directory
        file(GLOB LIB_CONTENTS "${DISCORD_SDK_PATH}/lib/*")
        message(STATUS "Contents of lib directory:")
        foreach(ITEM ${LIB_CONTENTS})
            message(STATUS "  ${ITEM}")
        endforeach()
        
        # Check specific library paths
        set(DEBUG_LIB "${DISCORD_SDK_PATH}/lib/debug/libdiscord_partner_sdk.so")
        set(RELEASE_LIB "${DISCORD_SDK_PATH}/lib/release/libdiscord_partner_sdk.so")
        
        message(STATUS "Checking library paths:")
        message(STATUS "  Debug lib: ${DEBUG_LIB}")
        if(EXISTS "${DEBUG_LIB}")
            message(STATUS "  ✓ Debug lib exists")
        else()
            message(STATUS "  ❌ Debug lib not found")
        endif()
        
        message(STATUS "  Release lib: ${RELEASE_LIB}")
        if(EXISTS "${RELEASE_LIB}")
            message(STATUS "  ✓ Release lib exists") 
        else()
            message(STATUS "  ❌ Release lib not found")
        endif()
        
    else()
        message(STATUS "❌ lib directory not found")
    endif()
    
else()
    message(STATUS "❌ Discord SDK directory does not exist: ${DISCORD_SDK_PATH}")
endif()

message(STATUS "=== END DISCORD SDK DETECTION ===")

# Source files
set(SOURCES
    src/main.cpp
    src/process_detector.cpp
    src/fl_studio_detector.cpp
    src/discord_client.cpp
    src/config.cpp
)

# Create executable
add_executable(${PROJECT_NAME} ${SOURCES})

# Link platform-specific libraries
target_link_libraries(${PROJECT_NAME} ${PLATFORM_LIBS})

# Now try to link Discord SDK
message(STATUS "=== LINKING DISCORD SDK ===")
if(NOT WIN32 AND NOT APPLE) # Linux
    set(DEBUG_LIB "${DISCORD_SDK_PATH}/lib/debug/libdiscord_partner_sdk.so")
    set(RELEASE_LIB "${DISCORD_SDK_PATH}/lib/release/libdiscord_partner_sdk.so")
    
    # Try release first, as it might have fewer dependencies
    if(EXISTS "${RELEASE_LIB}")
        target_link_libraries(${PROJECT_NAME} "${RELEASE_LIB}")
        message(STATUS "✓ Linked with release Discord SDK library")
    elseif(EXISTS "${DEBUG_LIB}")
        target_link_libraries(${PROJECT_NAME} "${DEBUG_LIB}")
        message(STATUS "✓ Linked with debug Discord SDK library")
    else()
        message(FATAL_ERROR "❌ No Discord SDK library found for Linux!")
    endif()
endif()

# Set output directory
set_target_properties(${PROJECT_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

message(STATUS "=== CMAKE DEBUG END ===")
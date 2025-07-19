cmake_minimum_required(VERSION 3.20)
project(FLStudioDiscordRPC VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Platform detection and libraries
if(WIN32)
    set(PLATFORM_LIBS user32 psapi)
    message(STATUS "Platform: Windows")
elseif(APPLE)
    find_library(CORE_FOUNDATION CoreFoundation)
    find_library(APPLICATION_SERVICES ApplicationServices)
    find_library(COCOA Cocoa)
    set(PLATFORM_LIBS ${CORE_FOUNDATION} ${APPLICATION_SERVICES} ${COCOA})
    set_source_files_properties(src/process_detector.cpp PROPERTIES COMPILE_FLAGS "-x objective-c++")
    message(STATUS "Platform: macOS")
else()
    set(PLATFORM_LIBS pthread dl)
    message(STATUS "Platform: Linux")
endif()

# Discord Partner SDK setup
set(DISCORD_SDK_PATH "${CMAKE_SOURCE_DIR}/discord_social_sdk")
message(STATUS "Discord SDK path: ${DISCORD_SDK_PATH}")

# Check if Discord SDK exists
if(NOT EXISTS "${DISCORD_SDK_PATH}")
    message(FATAL_ERROR "Discord SDK not found at ${DISCORD_SDK_PATH}. Please download and extract the Discord Partner SDK.")
endif()

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

# Add Discord SDK include directory
target_include_directories(${PROJECT_NAME} PRIVATE 
    "${DISCORD_SDK_PATH}/include"
    "${CMAKE_SOURCE_DIR}/include"
    "${CMAKE_SOURCE_DIR}/src"
)

# Link Discord Partner SDK library based on your file structure
if(WIN32)
    # Windows - link with .lib file and copy .dll
    target_link_libraries(${PROJECT_NAME} "${DISCORD_SDK_PATH}/lib/debug/discord_partner_sdk.lib")
    # Copy DLL to output directory
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${DISCORD_SDK_PATH}/bin/debug/discord_partner_sdk.dll"
        "$<TARGET_FILE_DIR:${PROJECT_NAME}>")
        
elseif(APPLE)
    # macOS - link with .dylib
    target_link_libraries(${PROJECT_NAME} "${DISCORD_SDK_PATH}/lib/debug/libdiscord_partner_sdk.dylib")
    # Set proper rpath for dylib
    set_target_properties(${PROJECT_NAME} PROPERTIES
        INSTALL_RPATH "@executable_path:${DISCORD_SDK_PATH}/lib/debug"
        BUILD_WITH_INSTALL_RPATH TRUE)
        
else() # Linux
    # Linux - link with .so
    # Try release version first, then debug as fallback
    if(EXISTS "${DISCORD_SDK_PATH}/lib/release/libdiscord_partner_sdk.so")
        target_link_libraries(${PROJECT_NAME} "${DISCORD_SDK_PATH}/lib/release/libdiscord_partner_sdk.so")
        set_target_properties(${PROJECT_NAME} PROPERTIES
            INSTALL_RPATH "$ORIGIN:${DISCORD_SDK_PATH}/lib/release"
            BUILD_WITH_INSTALL_RPATH TRUE)
    else()
        target_link_libraries(${PROJECT_NAME} "${DISCORD_SDK_PATH}/lib/debug/libdiscord_partner_sdk.so")
        set_target_properties(${PROJECT_NAME} PROPERTIES
            INSTALL_RPATH "$ORIGIN:${DISCORD_SDK_PATH}/lib/debug"
            BUILD_WITH_INSTALL_RPATH TRUE)
    endif()
    # Set RPATH so the executable can find the shared library
    set_target_properties(${PROJECT_NAME} PROPERTIES
        INSTALL_RPATH "$ORIGIN:${DISCORD_SDK_PATH}/lib/debug"
        BUILD_WITH_INSTALL_RPATH TRUE)
endif()

# Link platform-specific libraries
target_link_libraries(${PROJECT_NAME} ${PLATFORM_LIBS})

# Set output directory
set_target_properties(${PROJECT_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

# Debug output
message(STATUS "=== BUILD CONFIGURATION ===")
message(STATUS "Discord SDK include: ${DISCORD_SDK_PATH}/include")
message(STATUS "Discord SDK lib: ${DISCORD_SDK_PATH}/lib/debug/")
message(STATUS "Target: ${PROJECT_NAME}")
message(STATUS "===============================")
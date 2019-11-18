cmake_minimum_required(VERSION 3.3 FATAL_ERROR)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

find_package(WPEFramework REQUIRED)
find_package(${NAMESPACE}Plugins REQUIRED)
find_package(${NAMESPACE}Definitions REQUIRED)

set(PLUGIN_NAME AVS)
set(MODULE_NAME ${NAMESPACE}${PLUGIN_NAME})
string(TOLOWER ${NAMESPACE} STORAGE_DIRECTORY)

# TODO: port flags to Buildroot recipe
# TODO: set proper defaults
# TODO: Remove dev flags
set(PLUGIN_AVS_ENABLE_A4B OFF CACHE BOOL "Enable Alexa 4 Buisness (revoke authorisation)")
set(PLUGIN_AVS_ENABLE_SMART_SCREEN_SUPPORT OFF CACHE BOOL "Enable smart screen support")
set(PLUGIN_AVS_DEFAULT_ALEXA_CLIENT_CONFIG "/etc/avs/AlexaClientSDKConfig.json" CACHE STRING "Path to AlexaClientSDKConfig")
set(PLUGIN_AVS_DEFAULT_SMART_SCREEN_CONFIG "/etc/avs/SmartScreenSDKConfig.json" CACHE STRING "Path to SmartScreenSDKConfig")
set(PLUGIN_AVS_DEFAULT_LOG_LEVEL "DEBUG0" CACHE STRING "Default log level for the SDK")
set(PLUGIN_AVS_ENABLE_PORTAUDIO_SUPPORT ON CACHE BOOL "Enable audio input from PortAudio library")
set(PLUGIN_AVS_DEFAULT_AUDIOSOURCE "BluetoothRemoteControl" CACHE STRING "Callsign of plugin that provides voice input or PORTAUDIO")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fdiagnostics-color=always")

# hack make it common
add_definitions(-DRAPIDJSON_HAS_STDSTRING)
add_definitions(-DGSTREAMER_MEDIA_PLAYER)
add_definitions(-DENABLE_THUNDER_INPUT_MANAGER)

# translation ON/OFF -> true/false
if(PLUGIN_AVS_ENABLE_SMART_SCREEN_SUPPORT)
    set(PLUGIN_AVS_ENABLE_SMART_SCREEN true)
    add_definitions(-DENABLE_SMART_SCREEN)
else()
    set(PLUGIN_AVS_ENABLE_SMART_SCREEN false)
endif()

add_library(${MODULE_NAME}
    SHARED
        Module.cpp
        AVS.cpp
        AVSJsonRpc.cpp)

set_target_properties(${MODULE_NAME} PROPERTIES
    CXX_STANDARD 11
    CXX_STANDARD_REQUIRED YES)

target_include_directories(${MODULE_NAME}
    PUBLIC
        AVSClient/include/)

add_subdirectory("Authorization")
add_subdirectory("AVSClient")

target_link_libraries(${MODULE_NAME}
    PRIVATE
        ${NAMESPACE}Plugins::${NAMESPACE}Plugins
        ${NAMESPACE}Definitions::${NAMESPACE}Definitions
        AVSDevice)

if(PLUGIN_AVS_ENABLE_SMART_SCREEN_SUPPORT)
    target_link_libraries(${MODULE_NAME}
        PRIVATE
            SmartScreen)
endif()

install(TARGETS ${MODULE_NAME}
    DESTINATION lib/${STORAGE_DIRECTORY}/plugins)

add_subdirectory(Integration)

write_config(${PLUGIN_NAME})

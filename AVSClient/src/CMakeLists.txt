find_package(AlexaClientSDK REQUIRED)
find_package(GStreamer REQUIRED)
find_package(Threads REQUIRED)
find_package(WPEFramework REQUIRED)
find_package(Portaudio)

set(MODULE_NAME AVSCommon)

if(GSTREAMER_FOUND)
    add_definitions(-DGSTREAMER_MEDIA_PLAYER)
endif()

if(PORTAUDIO_FOUND)
    add_definitions(-DPORTAUDIO)
endif()

add_subdirectory(AVSDevice)

if(PLUGIN_AVS_ENABLE_SMART_SCREEN_SUPPORT)
    add_subdirectory(SmartScreen)
endif()
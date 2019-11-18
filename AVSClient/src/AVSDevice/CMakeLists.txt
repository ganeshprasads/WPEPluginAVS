set(MODULE_NAME AVSDevice)

set(WPEFRAMEWORK_PLUGIN_AVS_AVSDEVICE_SOURCES)
list(APPEND WPEFRAMEWORK_PLUGIN_AVS_AVSDEVICE_SOURCES
    ConnectionObserver.cpp
    ConsolePrinter.cpp
    ConsoleReader.cpp
    GuiRenderer.cpp
    InteractionManager.cpp
    KeywordObserver.cpp
    LocaleAssetsManager.cpp
    SampleApplication.cpp
    SampleEqualizerModeController.cpp
    UIManager.cpp
    UserInputManager.cpp
    ThunderVoiceHandler.cpp
    ../ThunderInputManager.cpp
    ../ThunderLogger.cpp
)

if(PLUGIN_AVS_ENABLE_A4B)
    # TODO: currently not supported?
    list(APPEND WPEFRAMEWORK_PLUGIN_AVS_AVSDEVICE_SOURCES RevokeAuthorizationObserver.cpp)
    add_definitions(-DENABLE_REVOKE_AUTH)
endif()

if(PORTAUDIO_FOUND)
    list(APPEND WPEFRAMEWORK_PLUGIN_AVS_AVSDEVICE_SOURCES PortAudioMicrophoneWrapper.cpp)
endif()

add_library(${MODULE_NAME} ${WPEFRAMEWORK_PLUGIN_AVS_AVSDEVICE_SOURCES})

set_target_properties(${MODULE_NAME} PROPERTIES
        CXX_STANDARD 11
        CXX_STANDARD_REQUIRED ON)

target_include_directories(${MODULE_NAME}
    PUBLIC
        ../../include/
        ../../include/AVSDevice
        AlexaClientSDK::AlexaClientSDK)

target_link_libraries(${MODULE_NAME}
    PRIVATE
        ${ALEXA_CLIENT_SDK_LIBRARIES}
        ${CMAKE_THREAD_LIBS_INIT}
        rt
        m
        asound
        atomic)

if(GSTREAMER_FOUND)
    target_include_directories(${MODULE_NAME} PUBLIC ${GSTREAMER_INCLUDES})
    target_link_libraries(${MODULE_NAME}
        PRIVATE
            ${GSTREAMER_LIBRARIES}
            MediaPlayer)
endif()

if(GSTREAMER_FOUND)
    add_definitions(-DGSTREAMER_MEDIA_PLAYER)
endif()

if(PORTAUDIO_FOUND)
    target_include_directories(${MODULE_NAME} PUBLIC Portaudio::Portaudio)
    target_link_libraries(${MODULE_NAME} PRIVATE ${PORTAUDIO_LIBRARIES})
endif()

install(TARGETS ${MODULE_NAME} DESTINATION lib/)

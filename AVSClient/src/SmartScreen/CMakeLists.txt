find_package(AlexaSmartScreenSDK REQUIRED)
find_package(APLCore REQUIRED)
find_package(Yoga REQUIRED)
find_package(Websocketpp REQUIRED)
find_package(Asio REQUIRED)

set(MODULE_NAME SmartScreen)

set(WPEFRAMEWORK_PLUGIN_AVS_SMARTSCREEN_SOURCES)
list(APPEND WPEFRAMEWORK_PLUGIN_AVS_SMARTSCREEN_SOURCES
    AplCoreConnectionManager.cpp
    AplCoreEngineSDKLogBridge.cpp
    AplCoreGuiRenderer.cpp
    AplCoreMetrics.cpp
    AplCoreTextMeasurement.cpp
    ConnectionObserver.cpp
    ConsolePrinter.cpp
    FocusBridge.cpp
    GUILogBridge.cpp
    GUI/GUIClient.cpp
    GUI/GUIManager.cpp
    JsonUIManager.cpp
    KeywordObserver.cpp
    LocaleAssetsManager.cpp
    SampleApplication.cpp
    SampleEqualizerModeController.cpp
    ThunderVoiceHandler.cpp
    ../ThunderLogger.cpp
)

if(PORTAUDIO_FOUND)
    list(APPEND WPEFRAMEWORK_PLUGIN_AVS_SMARTSCREEN_SOURCES PortAudioMicrophoneWrapper.cpp)
endif()

add_library(${MODULE_NAME} ${WPEFRAMEWORK_PLUGIN_AVS_SMARTSCREEN_SOURCES})

set_target_properties(${MODULE_NAME} PROPERTIES
        CXX_STANDARD 11
        CXX_STANDARD_REQUIRED ON)

target_include_directories(${MODULE_NAME}
    PUBLIC
        ../../include/
        ../../include/SmartScreen
        AlexaClientSDK::AlexaClientSDK
        AlexaSmartScreenSDK::AlexaSmartScreenSDK)

target_link_libraries(${MODULE_NAME}
    PRIVATE
        AlexaPresentation
        SmartScreenTemplateRunTime
        VisualCharacteristics
        Communication
        SmartScreenClient
        ${ALEXA_CLIENT_SDK_LIBRARIES}
        ${CMAKE_THREAD_LIBS_INIT}
        rt
        m
        asound
        atomic)

if(APL_CORE_FOUND)
    target_include_directories(${MODULE_NAME} PRIVATE APLCore::APLCore)
    add_definitions(-DAPL_CORE)
    # TODO: fPIC patch for static library was needed, find other way
    target_link_libraries(${MODULE_NAME} PRIVATE ${APL_CORE_LIBRARIES})
else()
    message(FATAL_ERROR "MISSING APL Core Library")
endif()

if(YOGA_FOUND)
    target_include_directories(${MODULE_NAME} PRIVATE Yoga::Yoga)
    # TODO: fpic patch nedded
    # target_link_libraries(${MODULE_NAME} PRIVATE ${YOGA_LIBRARIES})
else()
    message(FATAL_ERROR "MISSING Yoga Library")
endif()

if(WEBSOCKETPP_FOUND)
    target_include_directories(${MODULE_NAME} PRIVATE Websocketpp::Websocketpp)
else()
    message(FATAL_ERROR "MISSING Websocketpp Library")
endif()

if(ASIO_FOUND)
    target_include_directories(${MODULE_NAME} PRIVATE Asio::Asio)
    add_definitions(-DASIO_STANDALONE)
else()
    message(FATAL_ERROR "MISSING Asio (Standalone) Library")
endif()

if(GSTREAMER_FOUND)
    target_include_directories(${MODULE_NAME} PUBLIC ${GSTREAMER_INCLUDES})
    target_link_libraries(${MODULE_NAME}
        PRIVATE
            ${GSTREAMER_LIBRARIES}
            MediaPlayer)
endif()

if(PORTAUDIO_FOUND)
    target_include_directories(${MODULE_NAME} PUBLIC Portaudio::Portaudio)
    target_link_libraries(${MODULE_NAME} PRIVATE ${PORTAUDIO_LIBRARIES})
endif()

install(TARGETS ${MODULE_NAME} DESTINATION lib/)

# TODO: dont use hardcoded PLATFORM variable
set(PLATFORM "RaspberryPi3" CACHE STRING "Platform integration")

# platform independent files
install(DIRECTORY
    DESTINATION "${CMAKE_INSTALL_PREFIX}/../etc/avs"
    COMPONENT ${MODULE_NAME})

install(DIRECTORY
    DESTINATION "${CMAKE_INSTALL_PREFIX}/../etc/avs/db"
    COMPONENT ${MODULE_NAME})

# platform dependent files
install(FILES "${PLATFORM}/AlexaClientSDKConfig.json"
    DESTINATION "${CMAKE_INSTALL_PREFIX}/../etc/avs"
    COMPONENT ${MODULE_NAME})

install(FILES "${PLATFORM}/SmartScreenSDKConfig.json"
    DESTINATION "${CMAKE_INSTALL_PREFIX}/../etc/avs"
    COMPONENT ${MODULE_NAME})

install(FILES "${PLATFORM}/asound.conf"
    DESTINATION "${CMAKE_INSTALL_PREFIX}/../etc/"
    COMPONENT ${MODULE_NAME})

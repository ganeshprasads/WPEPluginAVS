 /*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2020 RDK Management
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "AVSDevice.h"

#if defined(KWD_PRYON)
#include "PryonKeywordDetector.h"
#endif

#include "ThunderLogger.h"
#include "ThunderVoiceHandler.h"
#include "TraceCategories.h"

#include <ACL/Transport/HTTP2TransportFactory.h>
#include <AVSCommon/AVS/Initialization/AlexaClientSDKInit.h>
#include <AVSCommon/Utils/LibcurlUtils/HTTPContentFetcherFactory.h>
#include <AVSCommon/Utils/LibcurlUtils/HttpPut.h>
#include <AVSCommon/Utils/LibcurlUtils/LibcurlHTTP2ConnectionFactory.h>
#include <AVSCommon/Utils/Logger/LoggerSinkManager.h>
#include <AVSCommon/Utils/Network/InternetConnectionMonitor.h>
#include  <acsdkAlerts/Storage/SQLiteAlertStorage.h>
#include <Audio/AudioFactory.h>
#include <CBLAuthDelegate/CBLAuthDelegate.h>
#include <CBLAuthDelegate/SQLiteCBLAuthDelegateStorage.h>
#include <ContextManager/ContextManager.h>
#include  <acsdkNotifications/SQLiteNotificationsStorage.h>
#include <SQLiteStorage/SQLiteMiscStorage.h>
#include <Settings/Storage/SQLiteDeviceSettingStorage.h>

#include <AVS/SampleApp/LocaleAssetsManager.h>
#include <AVS/SampleApp/PortAudioMicrophoneWrapper.h>

#include <cctype>
#include <fstream>

namespace WPEFramework {
namespace Plugin {

    SERVICE_REGISTRATION(AVSDevice, 1, 0);

    using namespace alexaClientSDK;

    // Alexa Client Config keys
    static const std::string SAMPLE_APP_CONFIG_KEY("sampleApp");
    static const std::string FIRMWARE_VERSION_KEY("firmwareVersion");
    static const std::string ENDPOINT_KEY("endpoint");
    static const std::string DISPLAY_CARD_KEY("displayCardsSupported");


    static const std::string AUDIO_MEDIAPLAYER_POOL_SIZE_KEY("audioMediaPlayerPoolSize");
    static const unsigned int AUDIO_MEDIAPLAYER_POOL_SIZE_DEFAULT = 2; 
	 // Share Data stream Configuraiton
    static const size_t MAX_READERS = 10;
    static const size_t WORD_SIZE = 2;
    static const unsigned int SAMPLE_RATE_HZ = 16000;
    static const unsigned int NUM_CHANNELS = 1;
    static const std::chrono::seconds AMOUNT_OF_AUDIO_DATA_IN_BUFFER = std::chrono::seconds(15);
    static const size_t BUFFER_SIZE_IN_SAMPLES = (SAMPLE_RATE_HZ)*AMOUNT_OF_AUDIO_DATA_IN_BUFFER.count();

    // Thunder voice handler
    static constexpr const char* PORTAUDIO_CALLSIGN("PORTAUDIO");

    bool AVSDevice::Initialize(PluginHost::IShell* service, const string& configuration)
    {
        
	TRACE_L1("Initializing AVSDevice...");

        Config config;
        bool status = true;

        ASSERT(service != nullptr);
        ASSERT(_service == nullptr);
        _service = service;

        config.FromString(configuration);
        const std::string logLevel = config.LogLevel.Value();
        if (logLevel.empty() == true) {
            TRACE(AVSClient, (_T("Missing log level")));
            status = false;
        } else {
            status = InitSDKLogs(logLevel);
        }

        const std::string alexaClientConfig = config.AlexaClientConfig.Value();
        if ((status == true) && (alexaClientConfig.empty() == true)) {
            TRACE(AVSClient, (_T("Missing AlexaClient config file")));
            status = false;
        }

        const std::string pathToInputFolder = config.KWDModelsPath.Value();
        if ((status == true) && (pathToInputFolder.empty() == true)) {
            TRACE(AVSClient, (_T("Missing KWD models path")));
            status = false;
        }

        const std::string audiosource = config.Audiosource.Value();
        if ((status == true) && (audiosource.empty() == true)) {
            TRACE(AVSClient, (_T("Missing audiosource")));
            status = false;
        }

        const bool enableKWD = config.EnableKWD.Value();
        if (enableKWD == true) {
#if !defined(KWD_PRYON)
            TRACE(AVSClient, (_T("Requested KWD, but it is not compiled in")));
            status = false;
#endif
        }

	if (status == true) {
            status = Init(audiosource, enableKWD, pathToInputFolder, alexaClientConfig);
        }

        return status;
    }

     bool AVSDevice::Init(const std::string& audiosource, const bool enableKWD, const std::string& pathToInputFolder, const std::string& alexaClientConfig)
    {
    using namespace alexaClientSDK::sampleApp; 
    using namespace alexaClientSDK::avsCommon::utils::mediaPlayer;
    using namespace alexaClientSDK::avsCommon::sdkInterfaces;   
    
    
    TRACE_L1("DEBUGLOG: Line count: 1, START");
    
    auto jsonConfig = std::make_shared<std::vector<std::shared_ptr<std::istream>>>();
    auto configInFile = std::shared_ptr<std::ifstream>(new std::ifstream(alexaClientConfig));
    if (!configInFile->good()) {
        TRACE(AVSClient, (_T("Failed to read config file filename")));
            return false;
        }

        jsonConfig->push_back(configInFile);
        
#if defined(KWD_PRYON)
    if (enableKWD) {
        auto localeToModelsConfig = pathToInputFolder + "/localeToModels.json";
        auto ltmConfigInFile = std::shared_ptr<std::ifstream>(new std::ifstream(localeToModelsConfig));
        if (!ltmConfigInFile->good()) {
            TRACE(AVSClient, (_T("Failed to read ltm config file filename")));
            return false;
        }
        jsonConfig->push_back(ltmConfigInFile);
    }
#endif
    
    auto avsBuilder = alexaClientSDK::avsCommon::avs::initialization::InitializationParametersBuilder::create();
    avsBuilder->withJsonStreams(jsonConfig);
    if (!avsBuilder) {
        TRACE(AVSClient, (_T("createInitializeParamsFailed reason nullBuilder")));
        return false;
    }

    auto params = avsBuilder->build();  
    alexaClientSDK::acsdkSampleApplication::SampleApplicationComponent avsAppComponent =
        acsdkSampleApplication::getComponent(std::move(params), m_shutdownRequiredList);
    auto avsAppFactory = alexaClientSDK::acsdkManufactory::Manufactory<
        std::shared_ptr<avsCommon::avs::initialization::AlexaClientSDKInit>,
        std::shared_ptr<avsCommon::sdkInterfaces::AuthDelegateInterface>,
        std::shared_ptr<avsCommon::sdkInterfaces::ContextManagerInterface>,
        std::shared_ptr<avsCommon::sdkInterfaces::LocaleAssetsManagerInterface>,
        std::shared_ptr<avsCommon::utils::DeviceInfo>,
        std::shared_ptr<avsCommon::utils::configuration::ConfigurationNode>,
        std::shared_ptr<avsCommon::utils::metrics::MetricRecorderInterface>,
        std::shared_ptr<registrationManager::CustomerDataManager>,
        std::shared_ptr<UIManager>>::create(avsAppComponent);

    m_sdkInit = avsAppFactory->get<std::shared_ptr<avsCommon::avs::initialization::AlexaClientSDKInit>>();
    if (!m_sdkInit) {
        TRACE(AVSClient, (_T("Failed to get SDKInit!")));
        return false;
    }
    
    auto configEntry = avsAppFactory->get<std::shared_ptr<avsCommon::utils::configuration::ConfigurationNode>>();
    if (!configEntry) {
        TRACE(AVSClient, (_T("Failed to acquire the configuration")));
        return false;
    }
    auto& config = *configEntry;
    auto appConfig = config[SAMPLE_APP_CONFIG_KEY];

    auto httpFactory = std::make_shared<avsCommon::utils::libcurlUtils::HTTPContentFetcherFactory>();

     std::shared_ptr<alexaClientSDK::storage::sqliteStorage::SQLiteMiscStorage> miscStorage =
         alexaClientSDK::storage::sqliteStorage::SQLiteMiscStorage::create(config);


    bool equalizerEnabled = false;

    auto speakerInterface = createApplicationMediaPlayer(httpFactory, false, "SpeakMediaPlayer");
    if (!speakerInterface) {
        TRACE(AVSClient, (_T("Failed to create application media interfaces for speech!")));
        return false;
    }
    m_speakMediaPlayer = speakerInterface->mediaPlayer;

    int poolSize;
    appConfig.getInt(AUDIO_MEDIAPLAYER_POOL_SIZE_KEY, &poolSize, AUDIO_MEDIAPLAYER_POOL_SIZE_DEFAULT);
    std::vector<std::shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::SpeakerInterface>> appAudioDevices;

    for (int index = 0; index < poolSize; index++) {
        // std::shared_ptr<MediaPlayerInterface> mediaPlayer;
        // std::shared_ptr<avsCommon::sdkInterfaces::SpeakerInterface> speaker;
        // std::shared_ptr<acsdkEqualizerInterfaces::EqualizerInterface> equalizer;

        auto audioInterface =
            createApplicationMediaPlayer(httpFactory, equalizerEnabled, "AudioMediaPlayer");
        if (!audioInterface) {
            TRACE(AVSClient, (_T("Failed to create application media interfaces for audio!")));
            return false;
        }
        m_audioMediaPlayerPool.push_back(audioInterface->mediaPlayer);
        appAudioDevices.push_back(audioInterface->speaker);
        
    }

    avsCommon::utils::Optional<avsCommon::utils::mediaPlayer::Fingerprint> fingerprint =
        (*(m_audioMediaPlayerPool.begin()))->getFingerprint();
    auto audioPlayerFactory = std::unique_ptr<mediaPlayer::PooledMediaPlayerFactory>();
    if (fingerprint.hasValue()) {
        audioPlayerFactory =
            mediaPlayer::PooledMediaPlayerFactory::create(m_audioMediaPlayerPool, fingerprint.value());
    } else {
        audioPlayerFactory = mediaPlayer::PooledMediaPlayerFactory::create(m_audioMediaPlayerPool);
    }
    if (!audioPlayerFactory) {
        TRACE(AVSClient, (_T("Failed to create media player factory for content!")));
        return false;
    }

    auto notificationInterface =
        createApplicationMediaPlayer(httpFactory, false, "NotificationsMediaPlayer");
    if (!notificationInterface) {
        TRACE(AVSClient, (_T("Failed to create application media interfaces for notifications!")));
        return false;
    }
    m_notificationsMediaPlayer = notificationInterface->mediaPlayer;

    auto bluetoothInterface =
        createApplicationMediaPlayer(httpFactory, false, "BluetoothMediaPlayer");
    if (!bluetoothInterface) {
        TRACE(AVSClient, (_T("Failed to create application media interfaces for bluetooth!")));
        return false;
    }
    m_bluetoothMediaPlayer = bluetoothInterface->mediaPlayer;

    auto ringtoneInterface =
        createApplicationMediaPlayer(httpFactory, false, "RingtoneMediaPlayer");
    if (!ringtoneInterface) {
        alexaClientSDK::sampleApp::ConsolePrinter::simplePrint(
            "Failed to create application media interfaces for ringtones!");
        return false;
    }
    m_ringtoneMediaPlayer = ringtoneInterface->mediaPlayer;


    auto alertInterface = createApplicationMediaPlayer(httpFactory, false, "AlertsMediaPlayer");
    if (!alertInterface) {
        TRACE(AVSClient, (_T("Failed to create application media interfaces for alerts!")));
        return false;
    }
    m_alertsMediaPlayer = alertInterface->mediaPlayer;

    auto systemAudioInterface =
        createApplicationMediaPlayer(httpFactory, false, "SystemSoundMediaPlayer");
    if (!systemAudioInterface) {
        TRACE(AVSClient, (_T("Failed to create application media interfaces for system sound player!")));
        return false;
    }
    m_systemSoundMediaPlayer = systemAudioInterface->mediaPlayer;



    auto appAudio = std::make_shared<alexaClientSDK::applicationUtilities::resources::audio::AudioFactory>();

    auto appAlertStorage =
        alexaClientSDK::acsdkAlerts::storage::SQLiteAlertStorage::create(config, appAudio->alerts());

    auto appMsgStorage = alexaClientSDK::certifiedSender::SQLiteMessageStorage::create(config);

    auto appNotifStorage = alexaClientSDK::acsdkNotifications::SQLiteNotificationsStorage::create(config);

    auto appdevSettingStorage = alexaClientSDK::settings::storage::SQLiteDeviceSettingStorage::create(config);

    auto appLocale = avsAppFactory->get<std::shared_ptr<LocaleAssetsManagerInterface>>();
    if (!appLocale) {
        TRACE(AVSClient, (_T("Failed to create Locale Assets Manager!")));
        return false;
    }

    auto appUI = avsAppFactory->get<std::shared_ptr<alexaClientSDK::sampleApp::UIManager>>();
    if (!appUI) {
        TRACE(AVSClient, (_T("Failed to get UIManager!")));
        return false;
    }

    auto appCustDataManager = avsAppFactory->get<std::shared_ptr<registrationManager::CustomerDataManager>>();
    if (!appCustDataManager) {
        TRACE(AVSClient, (_T("Failed to get CustomerDataManager!")));
        return false;
    }

    auto appDevInfo = avsAppFactory->get<std::shared_ptr<avsCommon::utils::DeviceInfo>>();
    if (!appDevInfo) {
        TRACE(AVSClient, (_T("Creation of DeviceInfo failed!")));
        return false;
    }

   
    alexaClientSDK::avsCommon::utils::uuidGeneration::setSalt(
        appDevInfo->getClientId() + appDevInfo->getDeviceSerialNumber());

   
    auto appAuthDelegate = avsAppFactory->get<std::shared_ptr<AuthDelegateInterface>>();
    if (!appAuthDelegate) {
        TRACE(AVSClient, (_T("Creation of AuthDelegate failed!")));
        return false;
    }

   
    auto appCDStorage =
        alexaClientSDK::capabilitiesDelegate::storage::SQLiteCapabilitiesDelegateStorage::create(config);

    m_capabilitiesDelegate = alexaClientSDK::capabilitiesDelegate::CapabilitiesDelegate::create(
        appAuthDelegate, std::move(appCDStorage), appCustDataManager);

    if (!m_capabilitiesDelegate) {
        TRACE(AVSClient, (_T("Creation of CapabilitiesDelegate failed!")));
        return false;
    }

    m_shutdownRequiredList.push_back(m_capabilitiesDelegate);
    appAuthDelegate->addAuthObserver(appUI);
    m_capabilitiesDelegate->addCapabilitiesObserver(appUI);

    int firmwareVersion = static_cast<int>(avsCommon::sdkInterfaces::softwareInfo::INVALID_FIRMWARE_VERSION);
    appConfig.getInt(FIRMWARE_VERSION_KEY, &firmwareVersion, firmwareVersion);

    bool displayCardsSupported;
    config[SAMPLE_APP_CONFIG_KEY].getBool(DISPLAY_CARD_KEY, &displayCardsSupported, true);

    auto appICMonitor =
        avsCommon::utils::network::InternetConnectionMonitor::create(httpFactory);
    if (!appICMonitor) {
        TRACE(AVSClient, (_T("Failed to create InternetConnectionMonitor")));
        return false;
    }

    
    auto appCtxtManager = avsAppFactory->get<std::shared_ptr<ContextManagerInterface>>();
    if (!appCtxtManager) {
        TRACE(AVSClient, (_T("Creation of ContextManager failed.")));
        return false;
    }

    auto appGWMStorage = avsGatewayManager::storage::AVSGatewayManagerStorage::create(miscStorage);
    if (!appGWMStorage) {
        TRACE(AVSClient, (_T("Creation of AVSGatewayManagerStorage failed")));
        return false;
    }
    auto appGWM =
        avsGatewayManager::AVSGatewayManager::create(std::move(appGWMStorage), appCustDataManager, config);
    if (!appGWM) {
        TRACE(AVSClient, (_T("Creation of AVSGatewayManager failed")));
        return false;
    }

    auto appSynStateFactory = synchronizeStateSender::SynchronizeStateSenderFactory::create(appCtxtManager);
    if (!appSynStateFactory) {
        TRACE(AVSClient, (_T("Creation of SynchronizeStateSenderFactory failed")));
        return false;
    }

    std::vector<std::shared_ptr<avsCommon::sdkInterfaces::PostConnectOperationProviderInterface>> providers;
    providers.push_back(appSynStateFactory);
    providers.push_back(appGWM);
    providers.push_back(m_capabilitiesDelegate);

    
    auto postConnectSequencerFactory = acl::PostConnectSequencerFactory::create(providers);


    auto httpTransport = std::make_shared<acl::HTTP2TransportFactory>(
        std::make_shared<avsCommon::utils::libcurlUtils::LibcurlHTTP2ConnectionFactory>(),
        postConnectSequencerFactory,
        nullptr,
        nullptr);

   
    size_t bufferSize = alexaClientSDK::avsCommon::avs::AudioInputStream::calculateBufferSize(
        BUFFER_SIZE_IN_SAMPLES, WORD_SIZE, MAX_READERS);
    auto tmpBuffer = std::make_shared<alexaClientSDK::avsCommon::avs::AudioInputStream::Buffer>(bufferSize);
    std::shared_ptr<alexaClientSDK::avsCommon::avs::AudioInputStream> sharedAudioStream =
        alexaClientSDK::avsCommon::avs::AudioInputStream::create(tmpBuffer, WORD_SIZE, MAX_READERS);

    if (!sharedAudioStream) {
        TRACE(AVSClient, (_T("Failed to create shared data stream!")));
        return false;
    }


    alexaClientSDK::avsCommon::utils::AudioFormat audioFormat;
    audioFormat.sampleRateHz = SAMPLE_RATE_HZ;
    audioFormat.sampleSizeInBits = WORD_SIZE * CHAR_BIT;
    audioFormat.numChannels = NUM_CHANNELS;
    audioFormat.endianness = alexaClientSDK::avsCommon::utils::AudioFormat::Endianness::LITTLE;
    audioFormat.encoding = alexaClientSDK::avsCommon::utils::AudioFormat::Encoding::LPCM;

    
    alexaClientSDK::capabilityAgents::aip::AudioProvider appTaptoTalkProvider(
        sharedAudioStream,
        audioFormat,
        alexaClientSDK::capabilityAgents::aip::ASRProfile::NEAR_FIELD,
        true,
        true,
        true);

    alexaClientSDK::capabilityAgents::aip::AudioProvider appHoldtoTalkProvider(
        sharedAudioStream,
        audioFormat,
        alexaClientSDK::capabilityAgents::aip::ASRProfile::CLOSE_TALK,
        false,
        true,
        false);

    auto metrics = avsAppFactory->get<std::shared_ptr<avsCommon::utils::metrics::MetricRecorderInterface>>();
 
    

std::shared_ptr<alexaClientSDK::defaultClient::DefaultClient> client =
        alexaClientSDK::defaultClient::DefaultClient::create(
            appDevInfo,
            appCustDataManager,
            m_externalMusicProviderMediaPlayersMap,
            m_externalMusicProviderSpeakersMap,
            m_adapterToCreateFuncMap,
            m_speakMediaPlayer,
            std::move(audioPlayerFactory),
            m_alertsMediaPlayer,
            m_notificationsMediaPlayer,
            m_bluetoothMediaPlayer,
            m_ringtoneMediaPlayer,
            m_systemSoundMediaPlayer,
            speakerInterface->speaker,
            appAudioDevices,
            alertInterface->speaker,
            notificationInterface->speaker,
            bluetoothInterface->speaker,
            ringtoneInterface->speaker,
            systemAudioInterface->speaker,
            {},
            nullptr, //equalizerRuntimeSetup,
            appAudio,
            appAuthDelegate,
            std::move(appAlertStorage),
            std::move(appMsgStorage),
            std::move(appNotifStorage),
            std::move(appdevSettingStorage),
            nullptr, //std::move(bluetoothStorage),
            miscStorage,
            {appUI},
            {appUI},
            std::move(appICMonitor),
            displayCardsSupported,
            m_capabilitiesDelegate,
            appCtxtManager,
            httpTransport,
            appGWM,
            appLocale,
            {}, //enabledConnectionRules,
            /* systemTimezone*/ nullptr,
            firmwareVersion,
            true,
            nullptr,
            nullptr, //std::move(bluetoothDeviceManager),
            metrics,
            nullptr,
            nullptr, //diagnostics,
            std::make_shared<alexaClientSDK::sampleApp::ExternalCapabilitiesBuilder>(appDevInfo),
            std::make_shared<alexaClientSDK::capabilityAgents::speakerManager::DefaultChannelVolumeFactory>(),
            true,
            std::make_shared<alexaClientSDK::acl::MessageRouterFactory>(),
            nullptr,
            appTaptoTalkProvider);


    if (!client) {
        TRACE(AVSClient, (_T("Failed to create default SDK client!")));
        return false;
    }
    
    client->addSpeakerManagerObserver(appUI);

    client->addNotificationsObserver(appUI);

    m_shutdownManager = client->getShutdownManager();
    if (!m_shutdownManager) {
        TRACE(AVSClient, (_T("Failed to get ShutdownManager!")));
        return false;
    }

    appUI->configureSettingsNotifications(client->getSettingsManager());

    /*
     * Add GUI Renderer as an observer if display cards are supported.
     */
    if (displayCardsSupported) {
        m_guiRenderer = std::make_shared<GuiRenderer>();
        client->addTemplateRuntimeObserver(m_guiRenderer);
    }

        // Audio input
        std::shared_ptr<applicationUtilities::resources::audio::MicrophoneInterface> aspInput = nullptr;
        std::shared_ptr<InteractionHandler<alexaClientSDK::sampleApp::InteractionManager>> aspInputInteractionHandler = nullptr;

        if (audiosource == PORTAUDIO_CALLSIGN) {
#if defined(PORTAUDIO)
            aspInput = sampleApp::PortAudioMicrophoneWrapper::create(sharedAudioStream);
#else
            TRACE(AVSClient, (_T("Portaudio support is not compiled in")));
            return false;
#endif
        } else {
            aspInputInteractionHandler = InteractionHandler<alexaClientSDK::sampleApp::InteractionManager>::Create();
            if (!aspInputInteractionHandler) {
                TRACE(AVSClient, (_T("Failed to create aspInputInteractionHandler")));
                return false;
            }

            m_thunderVoiceHandler = ThunderVoiceHandler<alexaClientSDK::sampleApp::InteractionManager>::create(sharedAudioStream, _service, audiosource, aspInputInteractionHandler, audioFormat);
            aspInput = m_thunderVoiceHandler;
            aspInput->startStreamingMicrophoneData();
        }
        if (!aspInput) {
            TRACE(AVSClient, (_T("Failed to create aspInput")));
            return false;
        }

#if defined(KWD_PRYON)
    if (enableKWD) {
    alexaClientSDK::capabilityAgents::aip::AudioProvider appWakeWordProvider(
        sharedAudioStream,
        audioFormat,
        alexaClientSDK::capabilityAgents::aip::ASRProfile::NEAR_FIELD,
        true,
        false,
        true);

    auto kwObserver = std::make_shared<alexaClientSDK::sampleApp::KeywordObserver>(client, appWakeWordProvider);

    m_keywordDetector = PryonKeywordDetector::create(
        sharedAudioStream,
        audioFormat,
        {kwObserver},
        std::unordered_set<
            std::shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::KeyWordDetectorStateObserverInterface>>(),
        pathToInputFolder);
    if (!m_keywordDetector) {
        TRACE(AVSClient, (_T("Failed to create keyword detector!")));
    }
        

    m_interactionManager = std::make_shared<alexaClientSDK::sampleApp::InteractionManager>(
        client,
        aspInput,
        appUI,
        appHoldtoTalkProvider,
        appTaptoTalkProvider,
        m_guiRenderer,
        appWakeWordProvider,
        nullptr,
        nullptr); //diagnostics);
}
#else
    m_interactionManager = std::make_shared<alexaClientSDK::sampleApp::InteractionManager>(
        client,
        aspInput,
        appUI,
        appHoldtoTalkProvider,
        appTaptoTalkProvider,
        m_guiRenderer,
        capabilityAgents::aip::AudioProvider::null(),
        nullptr,
        nullptr ); //diagnostics);
#endif

    m_shutdownRequiredList.push_back(m_interactionManager);
    client->addAlexaDialogStateObserver(m_interactionManager);
    client->addCallStateObserver(m_interactionManager);
    if (audiosource != PORTAUDIO_CALLSIGN) {
            if (aspInputInteractionHandler) {
                // register interactions that ThunderVoiceHandler may initiate
                if (!aspInputInteractionHandler->Initialize(m_interactionManager)) {
                    TRACE(AVSClient, (_T("Failed to initialize aspInputInteractionHandle")));
                    return false;
                }
            }
        }

    // Thunder Input Manager
    m_thunderInputManager = ThunderInputManager::create(m_interactionManager);
    if (!m_thunderInputManager) {
        TRACE(AVSClient, (_T("Failed to create m_thunderInputManager")));
        return false;
    }
    
    appAuthDelegate->addAuthObserver(m_thunderInputManager);
    client->addAlexaDialogStateObserver(m_thunderInputManager);
    client->getRegistrationManager()->addObserver(m_thunderInputManager);
    client->addTemplateRuntimeObserver(m_thunderInputManager);
    //client->addMessageObserver(m_thunderInputManager);
    m_capabilitiesDelegate->addCapabilitiesObserver(m_thunderInputManager);
    
    client->connect();
    TRACE_L1("DEBUGLOG: Line count: 1, END");
    return true;
    }

 
  
  bool AVSDevice::InitSDKLogs(const string& logLevel)
    {
        bool status = true;
        std::shared_ptr<avsCommon::utils::logger::Logger> thunderLogger = avsCommon::utils::logger::getThunderLogger();
        avsCommon::utils::logger::Level logLevelValue = avsCommon::utils::logger::Level::UNKNOWN;
        string logLevelUpper(logLevel);

        std::transform(logLevelUpper.begin(), logLevelUpper.end(), logLevelUpper.begin(), [](unsigned char c) { return std::toupper(c); });
        if (logLevelUpper.empty() == false) {
            logLevelValue = avsCommon::utils::logger::convertNameToLevel(logLevelUpper);
            if (avsCommon::utils::logger::Level::UNKNOWN == logLevelValue) {
                TRACE(AVSClient, (_T("Unknown log level")));
                status = false;
            }
        } else {
            status = false;
        }

        if (status == true) {
            TRACE(AVSClient, ((_T("Running app with log level: %s"), avsCommon::utils::logger::convertLevelToName(logLevelValue).c_str())));
            thunderLogger->setLevel(logLevelValue);
            avsCommon::utils::logger::LoggerSinkManager::instance().initialize(thunderLogger);
        }

        return status;
    }

    bool AVSDevice::JsonConfigToStream(std::vector<std::shared_ptr<std::istream>>& streams, const std::string& configFile)
    {
        if (configFile.empty()) {
            TRACE(AVSClient, (_T("Config filename is empty!")));
            return false;
        }

        auto configStream = std::shared_ptr<std::ifstream>(new std::ifstream(configFile));
        if (!configStream->good()) {
            TRACE(AVSClient, ((_T("Failed to read config file %s"), configFile.c_str())));
            return false;
        }

        streams.push_back(configStream);
        return true;
    }

    bool AVSDevice::Deinitialize()
    {
        TRACE_L1(_T("Deinitialize()"));

        return true;
    }

    WPEFramework::Exchange::IAVSController* AVSDevice::Controller()
    {
        if (m_thunderInputManager) {
            return m_thunderInputManager->Controller();
        } else {
            return nullptr;
        }
    }

    void AVSDevice::StateChange(WPEFramework::PluginHost::IShell* audiosource)
    {
        if (m_thunderVoiceHandler) {
            m_thunderVoiceHandler->stateChange(audiosource);
        }
    }
}
}

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

#include "SmartScreen.h"

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
//#include <Alerts/Storage/SQLiteAlertStorage.h>
#include <Audio/AudioFactory.h>
#include <CBLAuthDelegate/CBLAuthDelegate.h>
#include <CBLAuthDelegate/SQLiteCBLAuthDelegateStorage.h>
#include <ContextManager/ContextManager.h>
//#include <Notifications/SQLiteNotificationsStorage.h>
#include <SQLiteStorage/SQLiteMiscStorage.h>
#include <Settings/Storage/SQLiteDeviceSettingStorage.h>

#include <SmartScreen/Communication/WebSocketServer.h>



#include <acsdkAlerts/Storage/SQLiteAlertStorage.h>
#include <AVS/acsdkNotifications/SQLiteNotificationsStorage.h>
#include <APLClient/AplCoreGuiRenderer.h>

//#include <SmartScreen/SampleApp/AplCoreEngineSDKLogBridge.h>
//#include <SmartScreen/SampleApp/AplCoreGuiRenderer.h>
#include <SmartScreen/SampleApp/JsonUIManager.h>
#include <SmartScreen/SampleApp/KeywordObserver.h>
#include <SmartScreen/SampleApp/LocaleAssetsManager.h>
#include <SmartScreen/SampleApp/PortAudioMicrophoneWrapper.h>

#include <cctype>
#include <fstream>

#include <AVSCommon/AVS/Initialization/InitializationParametersBuilder.h>

#include <acsdkManufactory/Manufactory.h>
#include <ACL/Transport/HTTP2TransportFactory.h>
#include <ACL/Transport/PostConnectSequencerFactory.h>
#include <AVS/AVSCommon/AVS/CapabilitySemantics/CapabilitySemantics.h>
#include <AVS/AVSCommon/AVS/Initialization/InitializationParametersBuilder.h>
#include <AVS/AVSCommon/SDKInterfaces/PowerResourceManagerInterface.h>
#include <AVS/AVSCommon/Utils/LibcurlUtils/LibcurlHTTP2ConnectionFactory.h>
#include <AVS/AVSCommon/Utils/UUIDGeneration/UUIDGeneration.h>
#include <AVSGatewayManager/AVSGatewayManager.h>
#include <AVSGatewayManager/Storage/AVSGatewayManagerStorage.h>
#include <SynchronizeStateSender/SynchronizeStateSenderFactory.h>

#include "SmartScreen/SampleApp/ExternalCapabilitiesBuilder.h"
#include "SmartScreen/SampleApp/KeywordObserver.h"
#include "SmartScreen/SampleApp/LocaleAssetsManager.h"
#include "SmartScreen/SampleApp/SampleApplication.h"
#include "SmartScreen/SampleApp/SampleApplicationComponent.h"
#include "SmartScreen/SampleApp/SmartScreenCaptionPresenter.h"


#include <AVS/AVSCommon/AVS/Initialization/AlexaClientSDKInit.h>
#include <AVS/AVSCommon/SDKInterfaces/Bluetooth/BluetoothDeviceConnectionRuleInterface.h>
#include <AVS/AVSCommon/SDKInterfaces/Bluetooth/BluetoothDeviceManagerInterface.h>
#include <AVS/AVSCommon/SDKInterfaces/Diagnostics/ProtocolTracerInterface.h>
#include <AVS/AVSCommon/SDKInterfaces/Endpoints/EndpointBuilderInterface.h>
#include <AVS/AVSCommon/Utils/Configuration/ConfigurationNode.h>
#include <AVS/AVSCommon/Utils/DeviceInfo.h>
#include <AVS/AVSCommon/Utils/LibcurlUtils/HTTPContentFetcherFactory.h>
#include <AVS/AVSCommon/Utils/Logger/Logger.h>
#include <AVS/AVSCommon/Utils/Logger/LoggerSinkManager.h>
#include <AVS/AVSCommon/Utils/Network/InternetConnectionMonitor.h>
#include <acsdkAlerts/Storage/SQLiteAlertStorage.h>
#include <Audio/AudioFactory.h>
#include <Audio/MicrophoneInterface.h>
#include <acsdkBluetooth/BasicDeviceConnectionRule.h>
#include <acsdkBluetooth/SQLiteBluetoothStorage.h>
#include <acsdkNotifications/SQLiteNotificationsStorage.h>
#include <CBLAuthDelegate/CBLAuthDelegate.h>
#include <CBLAuthDelegate/SQLiteCBLAuthDelegateStorage.h>
#include <CapabilitiesDelegate/CapabilitiesDelegate.h>
#include <CapabilitiesDelegate/Storage/SQLiteCapabilitiesDelegateStorage.h>
#include <SQLiteStorage/SQLiteMiscStorage.h>
#include <SmartScreen/SampleApp/SampleEqualizerModeController.h>
#include <Settings/Storage/SQLiteDeviceSettingStorage.h>
#include <SmartScreen/Communication/WebSocketServer.h>
namespace WPEFramework {
namespace Plugin {

    SERVICE_REGISTRATION(SmartScreen, 1, 0);

    using namespace alexaClientSDK;

    // Alexa Client Config keys
    static const std::string SAMPLE_APP_CONFIG_KEY("sampleApp");
    static const std::string FIRMWARE_VERSION_KEY("firmwareVersion");
    static const std::string ENDPOINT_KEY("endpoint");

    static const std::string AUDIO_MEDIAPLAYER_POOL_SIZE_KEY("audioMediaPlayerPoolSize");
    static const unsigned int AUDIO_MEDIAPLAYER_POOL_SIZE_DEFAULT = 2; 

   
   // static const std::string WEBSOCKET_INTERFACE_KEY("websocketInterface");
   // static const std::string WEBSOCKET_PORT_KEY("websocketPort");
    static const std::string WEBSOCKET_CERTIFICATE("websocketCertificate");
    static const std::string WEBSOCKET_PRIVATE_KEY("websocketPrivateKey");
    static const std::string WEBSOCKET_CERTIFICATE_AUTHORITY("websocketCertificateAuthority");
//    static const std::string AUDIO_MEDIAPLAYER_POOL_SIZE_KEY("audioMediaPlayerPoolSize");
    static const std::string CONTENT_CACHE_REUSE_PERIOD_IN_SECONDS_KEY("contentCacheReusePeriodInSeconds");
    static const std::string DEFAULT_CONTENT_CACHE_REUSE_PERIOD_IN_SECONDS("600");
    static const std::string CONTENT_CACHE_MAX_SIZE_KEY("contentCacheMaxSize");
    static const std::string DEFAULT_CONTENT_CACHE_MAX_SIZE("50");
    static const std::string MAX_NUMBER_OF_CONCURRENT_DOWNLOAD_CONFIGURATION_KEY = "maxNumberOfConcurrentDownloads";
    static const int DEFAULT_MAX_NUMBER_OF_CONCURRENT_DOWNLOAD = 5;
     
    
    // Share Data stream Configuraiton
    static const size_t MAX_READERS = 10;
    static const size_t WORD_SIZE = 2;
    static const unsigned int SAMPLE_RATE_HZ = 16000;
    static const unsigned int NUM_CHANNELS = 1;
    static const std::chrono::seconds AMOUNT_OF_AUDIO_DATA_IN_BUFFER = std::chrono::seconds(15);
    static const size_t BUFFER_SIZE_IN_SAMPLES = (SAMPLE_RATE_HZ)*AMOUNT_OF_AUDIO_DATA_IN_BUFFER.count();

    // Thunder voice handler
    static constexpr const char* PORTAUDIO_CALLSIGN("PORTAUDIO");

    // smart screein
    static const std::string WEBSOCKET_INTERFACE_KEY("websocketInterface");
    static const std::string WEBSOCKET_PORT_KEY("websocketPort");
    static const std::string DEFAULT_WEBSOCKET_INTERFACE = "127.0.0.1";
    static const int DEFAULT_WEBSOCKET_PORT = 8933;

    
    bool SmartScreen::Initialize(PluginHost::IShell* service, const string& configuration)
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
        
    const std::string smartScreenConfig = config.SmartScreenConfig.Value();
        if ((status == true) && (smartScreenConfig.empty() == true)) {
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
            status = Init(audiosource, enableKWD, pathToInputFolder, alexaClientConfig, smartScreenConfig);
        }
        return status;
}

  bool SmartScreen::Init(const std::string& audiosource, const bool enableKWD, const std::string& pathToInputFolder, const std::string alexaClientConfig, const std::string smartScreenConfig)
    {
    using namespace alexaSmartScreenSDK::sampleApp; 
    using namespace alexaClientSDK::avsCommon::utils::mediaPlayer;
    using namespace alexaClientSDK::avsCommon::sdkInterfaces;   
    
    
    auto jsonConfig = std::make_shared<std::vector<std::shared_ptr<std::istream>>>();
    
    auto configInFile = std::shared_ptr<std::ifstream>(new std::ifstream(alexaClientConfig));
    if (!configInFile->good()) {
        TRACE(AVSClient, (_T("Failed to read appConfig file filename")));
            return false;
        }
    jsonConfig->push_back(configInFile);
        auto ssConfig = std::shared_ptr<std::ifstream>(new std::ifstream(smartScreenConfig));
        if (!ssConfig->good()) {
        TRACE(AVSClient, (_T("Failed to read smart screen appConfig file filename")));
            return false;
        }
        jsonConfig->push_back(ssConfig);

#if defined(KWD_PRYON)
    if (enableKWD) {
        auto localeToModelsConfig = pathToInputFolder + "/localeToModels.json";
        auto ltmConfigInFile = std::shared_ptr<std::ifstream>(new std::ifstream(localeToModelsConfig));
        if (!ltmConfigInFile->good()) {
            TRACE(AVSClient, (_T("Failed to read ltm appConfig file filename")));
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
    auto avsAppComponent =
        getComponent(std::move(params), m_shutdownRequiredList);
    auto avsAppFactory = alexaClientSDK::acsdkManufactory::Manufactory<
        std::shared_ptr<avsCommon::avs::initialization::AlexaClientSDKInit>,
        std::shared_ptr<avsCommon::sdkInterfaces::ContextManagerInterface>,
        std::shared_ptr<avsCommon::sdkInterfaces::LocaleAssetsManagerInterface>,
        std::shared_ptr<avsCommon::utils::configuration::ConfigurationNode>,
        std::shared_ptr<avsCommon::utils::DeviceInfo>,
        std::shared_ptr<registrationManager::CustomerDataManager>,
        std::shared_ptr<avsCommon::utils::metrics::MetricRecorderInterface>>::create(avsAppComponent);
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
    auto& appConfig = *configEntry;
    auto config = appConfig[SAMPLE_APP_CONFIG_KEY];

    auto httpFactory = std::make_shared<avsCommon::utils::libcurlUtils::HTTPContentFetcherFactory>();
     std::shared_ptr<alexaClientSDK::storage::sqliteStorage::SQLiteMiscStorage> miscStorage =
         alexaClientSDK::storage::sqliteStorage::SQLiteMiscStorage::create(appConfig);

    bool equalizerEnabled = false;

    auto speakerInterface = createApplicationMediaPlayer(httpFactory, false, "SpeakMediaPlayer");
    if (!speakerInterface) {
        TRACE(AVSClient, (_T("Failed to create application media interfaces for speech!")));
        return false;
    }
    m_speakMediaPlayer = speakerInterface->mediaPlayer;
    int poolSize;
    config.getInt(AUDIO_MEDIAPLAYER_POOL_SIZE_KEY, &poolSize, AUDIO_MEDIAPLAYER_POOL_SIZE_DEFAULT);
    std::vector<std::shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::SpeakerInterface>> audioDevices;
    for (int index = 0; index < poolSize; index++) {

        auto audioMediaInterfaces =
            createApplicationMediaPlayer(httpFactory, equalizerEnabled, "AudioMediaPlayer");
        if (!audioMediaInterfaces) {
            TRACE(AVSClient, (_T("Failed to create application media interfaces for audio!")));
            return false;
        }
        m_audioMediaPlayerPool.push_back(audioMediaInterfaces->mediaPlayer);
        audioDevices.push_back(audioMediaInterfaces->speaker);
    }

    avsCommon::utils::Optional<avsCommon::utils::mediaPlayer::Fingerprint> fingerprint =
        (*(m_audioMediaPlayerPool.begin()))->getFingerprint();
    auto appAudioPlayerFactory = std::unique_ptr<mediaPlayer::PooledMediaPlayerFactory>();
    if (fingerprint.hasValue()) {
        appAudioPlayerFactory =
            mediaPlayer::PooledMediaPlayerFactory::create(m_audioMediaPlayerPool, fingerprint.value());
    } else {
        appAudioPlayerFactory = mediaPlayer::PooledMediaPlayerFactory::create(m_audioMediaPlayerPool);
    }
    if (!appAudioPlayerFactory) {
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
    auto btInterface =
        createApplicationMediaPlayer(httpFactory, false, "BluetoothMediaPlayer");
    if (!btInterface) {
        TRACE(AVSClient, (_T("Failed to create application media interfaces for bluetooth!")));
        return false;
    }
    m_bluetoothMediaPlayer = btInterface->mediaPlayer;
    auto rtInterface =
        createApplicationMediaPlayer(httpFactory, false, "RingtoneMediaPlayer");
    if (!rtInterface) {
        TRACE(AVSClient, (_T("Failed to create application media interfaces for ringtones!")));
        return false;
    }
    m_ringtoneMediaPlayer = rtInterface->mediaPlayer;


    auto alertInterface = createApplicationMediaPlayer(httpFactory, false, "AlertsMediaPlayer");
    if (!alertInterface) {
        TRACE(AVSClient, (_T("Failed to create application media interfaces for alerts!")));
        return false;
    }
    m_alertsMediaPlayer = alertInterface->mediaPlayer;
    auto appSystemAudioInterface =
        createApplicationMediaPlayer(httpFactory, false, "SystemSoundMediaPlayer");
    if (!appSystemAudioInterface) {
        TRACE(AVSClient, (_T("Failed to create application media interfaces for system sound player!")));
        return false;
    }
    m_systemSoundMediaPlayer = appSystemAudioInterface->mediaPlayer;


    auto appAudioFactory = std::make_shared<alexaClientSDK::applicationUtilities::resources::audio::AudioFactory>();
    auto alertStorage =
        alexaClientSDK::acsdkAlerts::storage::SQLiteAlertStorage::create(appConfig, appAudioFactory->alerts());
    auto appMsgStorage = alexaClientSDK::certifiedSender::SQLiteMessageStorage::create(appConfig);
    auto appNotifStorage = alexaClientSDK::acsdkNotifications::SQLiteNotificationsStorage::create(appConfig);
    
    auto appDevSettingStorage = alexaClientSDK::settings::storage::SQLiteDeviceSettingStorage::create(appConfig);
    
    auto appLocaleManager = avsAppFactory->get<std::shared_ptr<LocaleAssetsManagerInterface>>();
    if (!appLocaleManager) {
        TRACE(AVSClient, (_T("Failed to create Locale Assets Manager!")));
        return false;
    }
    
    std::string APLVersion;
    std::string websocketInterface;
    appConfig.getString(WEBSOCKET_INTERFACE_KEY, &websocketInterface, DEFAULT_WEBSOCKET_INTERFACE);
    int websocketPortNumber;
    appConfig.getInt(WEBSOCKET_PORT_KEY, &websocketPortNumber, DEFAULT_WEBSOCKET_PORT);

#ifdef UWP_BUILD
    auto webSocketServer = std::make_shared<NullSocketServer>();
#else
    auto webSocketServer = std::make_shared<alexaSmartScreenSDK::communication::WebSocketServer>(websocketInterface, websocketPortNumber);
#ifdef ENABLE_WEBSOCKET_SSL
    std::string sslCaFile;
    appConfig.getString(WEBSOCKET_CERTIFICATE_AUTHORITY, &sslCaFile);
    std::string sslCertificateFile;
    appConfig.getString(WEBSOCKET_CERTIFICATE, &sslCertificateFile);

    std::string sslPrivateKeyFile;
    appConfig.getString(WEBSOCKET_PRIVATE_KEY, &sslPrivateKeyFile);

    webSocketServer->setCertificateFile(sslCaFile, sslCertificateFile, sslPrivateKeyFile);
#endif  // ENABLE_WEBSOCKET_SSL

#endif  // UWP_BUILD

    auto appCustDataManager = avsAppFactory->get<std::shared_ptr<registrationManager::CustomerDataManager>>();
    if (!appCustDataManager) {
        TRACE(AVSClient, (_T("Failed to get CustomerDataManager!")));
        return false;
    }
     m_guiClient = gui::GUIClient::create(webSocketServer, miscStorage, appCustDataManager);
    if (!m_guiClient) {
        TRACE(AVSClient, (_T("Creation of GUIClient failed!")));
        return false;
    }
    std::string cachePeriodInSeconds;
    std::string maxCacheSize;

    appConfig.getString(
        CONTENT_CACHE_REUSE_PERIOD_IN_SECONDS_KEY,
        &cachePeriodInSeconds,
        DEFAULT_CONTENT_CACHE_REUSE_PERIOD_IN_SECONDS);
    appConfig.getString(CONTENT_CACHE_MAX_SIZE_KEY, &maxCacheSize, DEFAULT_CONTENT_CACHE_MAX_SIZE);
    auto appContDwlManager = std::make_shared<CachingDownloadManager>(
        httpFactory,
        std::stol(cachePeriodInSeconds),
        std::stol(maxCacheSize),
        miscStorage,
        appCustDataManager);
    int maxConcDwls;
    appConfig.getInt(
        MAX_NUMBER_OF_CONCURRENT_DOWNLOAD_CONFIGURATION_KEY,
        &maxConcDwls,
        DEFAULT_MAX_NUMBER_OF_CONCURRENT_DOWNLOAD);

    if (1 > maxConcDwls) {
        maxConcDwls = DEFAULT_MAX_NUMBER_OF_CONCURRENT_DOWNLOAD;
        TRACE(AVSClient, (_T("Invalid values for maxConcDwls")));
    }
    auto aplParams = AplClientBridgeParameter{maxConcDwls};
    auto aplClientBridge = AplClientBridge::create(appContDwlManager, m_guiClient, aplParams);
    m_guiClient->setAplClientBridge(aplClientBridge);

    
    auto captionPresenter = std::make_shared<alexaSmartScreenSDK::sampleApp::SmartScreenCaptionPresenter>(m_guiClient);

    auto appDevInfo = avsAppFactory->get<std::shared_ptr<avsCommon::utils::DeviceInfo>>();
    if (!appDevInfo) {
        TRACE(AVSClient, (_T("Creation of DeviceInfo failed!")));
        return false;
    }

    auto appUI = std::make_shared<alexaSmartScreenSDK::sampleApp::JsonUIManager>(
        std::static_pointer_cast<alexaSmartScreenSDK::smartScreenSDKInterfaces::GUIClientInterface>(m_guiClient),
        appDevInfo);
    m_guiClient->setObserver(appUI);
    APLVersion = m_guiClient->getMaxAPLVersion();
    
    alexaClientSDK::avsCommon::utils::uuidGeneration::setSalt(
        appDevInfo->getClientId() + appDevInfo->getDeviceSerialNumber());
    
    auto appAuthDelStorage = authorization::cblAuthDelegate::SQLiteCBLAuthDelegateStorage::create(appConfig);
    std::shared_ptr<avsCommon::sdkInterfaces::AuthDelegateInterface> delAuth =
        authorization::cblAuthDelegate::CBLAuthDelegate::create(
            appConfig, appCustDataManager, std::move(appAuthDelStorage), appUI, nullptr, appDevInfo);

    if (!delAuth) {
        TRACE(AVSClient, (_T("Creation of AuthDelegate failed!")));
        return false;
    }
   
    auto appCDStorage =
        alexaClientSDK::capabilitiesDelegate::storage::SQLiteCapabilitiesDelegateStorage::create(appConfig);
    m_capabilitiesDelegate = alexaClientSDK::capabilitiesDelegate::CapabilitiesDelegate::create(
        delAuth, std::move(appCDStorage), appCustDataManager);
    if (!m_capabilitiesDelegate) {
         TRACE(AVSClient, (_T("Creation of CapabilitiesDelegate failed!")));
        return false;
    }
    m_shutdownRequiredList.push_back(m_capabilitiesDelegate);
    delAuth->addAuthObserver(appUI);
    m_capabilitiesDelegate->addCapabilitiesObserver(appUI);

    int firmwareVersion = static_cast<int>(avsCommon::sdkInterfaces::softwareInfo::INVALID_FIRMWARE_VERSION);
    config.getInt(FIRMWARE_VERSION_KEY, &firmwareVersion, firmwareVersion);
    
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
    auto appGWManager =
        avsGatewayManager::AVSGatewayManager::create(std::move(appGWMStorage), appCustDataManager, appConfig);
    if (!appGWManager) {
        TRACE(AVSClient, (_T("Creation of AVSGatewayManager failed")));
        return false;
    }
    auto appSyncFactory = synchronizeStateSender::SynchronizeStateSenderFactory::create(appCtxtManager);
    if (!appSyncFactory) {
        TRACE(AVSClient, (_T("Creation of SynchronizeStateSenderFactory failed")));
        return false;
    }
    

    std::vector<std::shared_ptr<avsCommon::sdkInterfaces::PostConnectOperationProviderInterface>> providers;
    providers.push_back(appSyncFactory);
    providers.push_back(appGWManager);
    providers.push_back(m_capabilitiesDelegate);
    
    auto postConnectSequencerFactory = acl::PostConnectSequencerFactory::create(providers);
   
    auto appHttpTransport = std::make_shared<acl::HTTP2TransportFactory>(
        std::make_shared<avsCommon::utils::libcurlUtils::LibcurlHTTP2ConnectionFactory>(),
        postConnectSequencerFactory,
        nullptr,
        nullptr);
    
    size_t bufferSize = alexaClientSDK::avsCommon::avs::AudioInputStream::calculateBufferSize(
        BUFFER_SIZE_IN_SAMPLES, WORD_SIZE, MAX_READERS);
    auto tmpBuffer = std::make_shared<alexaClientSDK::avsCommon::avs::AudioInputStream::Buffer>(bufferSize);
    std::shared_ptr<alexaClientSDK::avsCommon::avs::AudioInputStream> sharedDataStream =
        alexaClientSDK::avsCommon::avs::AudioInputStream::create(tmpBuffer, WORD_SIZE, MAX_READERS);
    if (!sharedDataStream) {
        TRACE(AVSClient, (_T("Failed to create shared data stream!")));
        return false;
    }
    

    alexaClientSDK::avsCommon::utils::AudioFormat appAudioFromat;
    appAudioFromat.sampleRateHz = SAMPLE_RATE_HZ;
    appAudioFromat.sampleSizeInBits = WORD_SIZE * CHAR_BIT;
    appAudioFromat.numChannels = NUM_CHANNELS;
    appAudioFromat.endianness = alexaClientSDK::avsCommon::utils::AudioFormat::Endianness::LITTLE;
    appAudioFromat.encoding = alexaClientSDK::avsCommon::utils::AudioFormat::Encoding::LPCM;
    appAudioFromat.dataSigned = false;
    
    
    alexaClientSDK::capabilityAgents::aip::AudioProvider appTapAudioProv(
        sharedDataStream,
        appAudioFromat,
        alexaClientSDK::capabilityAgents::aip::ASRProfile::NEAR_FIELD,
        true,
        true,
        true);
    
    alexaClientSDK::capabilityAgents::aip::AudioProvider appHoldAudioProv(
        sharedDataStream,
        appAudioFromat,
        alexaClientSDK::capabilityAgents::aip::ASRProfile::CLOSE_TALK,
        false,
        true,
        false);
        
    auto appMetrics = avsAppFactory->get<std::shared_ptr<avsCommon::utils::metrics::MetricRecorderInterface>>();
    
    
        // Audio input
        std::shared_ptr<applicationUtilities::resources::audio::MicrophoneInterface> aspInput = nullptr;
        std::shared_ptr<InteractionHandler<alexaSmartScreenSDK::sampleApp::gui::GUIManager>> aspInputInteractionHandler = nullptr;

        if (audiosource == PORTAUDIO_CALLSIGN) {
#if defined(PORTAUDIO)
            aspInput = alexaSmartScreenSDK::sampleApp::PortAudioMicrophoneWrapper::create(sharedDataStream);
#else
            TRACE(AVSClient, (_T("Portaudio support is not compiled in")));
            return false;
#endif
        } else {
            aspInputInteractionHandler = InteractionHandler<alexaSmartScreenSDK::sampleApp::gui::GUIManager>::Create();
            if (!aspInputInteractionHandler) {
                TRACE(AVSClient, (_T("Failed to create aspInputInteractionHandler")));
                return false;
            }
            m_thunderVoiceHandler = ThunderVoiceHandler<alexaSmartScreenSDK::sampleApp::gui::GUIManager>::create(sharedDataStream, _service, audiosource, aspInputInteractionHandler, appAudioFromat);
            aspInput = m_thunderVoiceHandler;
            aspInput->startStreamingMicrophoneData();
        }
        if (!aspInput) {
            TRACE(AVSClient, (_T("Failed to create aspInput")));
            return false;
        }
       
      alexaClientSDK::capabilityAgents::aip::AudioProvider
      appWakeAudioProv(capabilityAgents::aip::AudioProvider::null());
      
// Creating wake word audio provider, if necessary
#if defined(KWD_PRYON)
    if (enableKWD) {
    appWakeAudioProv = alexaClientSDK::capabilityAgents::aip::AudioProvider(
        sharedDataStream,
        appAudioFromat,
        alexaClientSDK::capabilityAgents::aip::ASRProfile::NEAR_FIELD,
        true,
        false,
        true);
    
    
        m_guiManager = alexaSmartScreenSDK::sampleApp::gui::GUIManager::create(
        m_guiClient,
#ifdef ENABLE_PCC
        phoneCaller,
#endif
        appHoldAudioProv,
        appTapAudioProv,
        aspInput,
        appWakeAudioProv);
}
#else
   
    m_guiManager = alexaSmartScreenSDK::sampleApp::gui::GUIManager::create(
        m_guiClient,
#ifdef ENABLE_PCC
        phoneCaller,
#endif
        appHoldAudioProv,
        appTapAudioProv,
        aspInput,
        alexaClientSDK::capabilityAgents::aip::AudioProvider::null());
        

#endif
    if (audiosource != PORTAUDIO_CALLSIGN) {
            if (aspInputInteractionHandler) {
                // register interactions that ThunderVoiceHandler may initiate
                if (!aspInputInteractionHandler->Initialize(m_guiManager)) {
                    TRACE(AVSClient, (_T("Failed to initialize aspInputInteractionHandle")));
                    return false;
                }
            }
        }
        
    
    std::shared_ptr<alexaSmartScreenSDK::smartScreenClient::SmartScreenClient> client = alexaSmartScreenSDK::smartScreenClient::SmartScreenClient::create(
        appDevInfo,
        appCustDataManager,
        m_externalMusicProviderMediaPlayersMap,
        m_externalMusicProviderSpeakersMap,
        m_adapterToCreateFuncMap,
        m_speakMediaPlayer,
        std::move(appAudioPlayerFactory),
        m_alertsMediaPlayer,
        m_notificationsMediaPlayer,
        m_bluetoothMediaPlayer,
        m_ringtoneMediaPlayer,
        m_systemSoundMediaPlayer,
        speakerInterface->speaker,
        audioDevices,
        alertInterface->speaker,
        notificationInterface->speaker,
        btInterface->speaker,
        rtInterface->speaker,
        appSystemAudioInterface->speaker,
        {},
        nullptr, //equalizerRuntimeSetup,
        appAudioFactory,
        delAuth,
        std::move(alertStorage),
        std::move(appMsgStorage),
        std::move(appNotifStorage),
        std::move(appDevSettingStorage),
        nullptr, //std::move(bluetoothStorage),
        miscStorage,
        {appUI},
        {appUI},
        std::move(appICMonitor),
        m_capabilitiesDelegate,
        appCtxtManager,
        appHttpTransport,
        appGWManager,
        appLocaleManager,
        {}, //enabledConnectionRules,
        /* systemTimezone*/ nullptr,
        firmwareVersion,
        true,
        nullptr,
        nullptr, //std::move(bluetoothDeviceManager),
        appMetrics,
        nullptr,
        nullptr, //diagnostics,
        std::make_shared<ExternalCapabilitiesBuilder>(appDevInfo),
        std::make_shared<alexaClientSDK::capabilityAgents::speakerManager::DefaultChannelVolumeFactory>(),
        true,
        std::make_shared<alexaClientSDK::acl::MessageRouterFactory>(),
        nullptr,
        capabilityAgents::aip::AudioProvider::null(),
        m_guiManager,
        APLVersion);
    if (!client) {
        TRACE(AVSClient, (_T("Failed to create default SDK client!")));
        return false;
    }
    
#if defined(KWD_PRYON)
    if (enableKWD) {    
    auto kwObserver = std::make_shared<alexaSmartScreenSDK::sampleApp::KeywordObserver>(client, appWakeAudioProv);
    m_keywordDetector = PryonKeywordDetector::create(
        sharedDataStream,
        appAudioFromat,
        {kwObserver},
        std::unordered_set<
            std::shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::KeyWordDetectorStateObserverInterface>>(),
        pathToInputFolder);
    if (!m_keywordDetector) {
        TRACE(AVSClient, (_T("Failed to create keyword detector!")));
    }
        
    }
#endif

    client->addSpeakerManagerObserver(appUI);
    client->addNotificationsObserver(appUI);
    client->addTemplateRuntimeObserver(m_guiManager);
    client->addAlexaPresentationObserver(m_guiManager);
    client->addAlexaDialogStateObserver(m_guiManager);
    client->addAudioPlayerObserver(m_guiManager);
    client->addAudioPlayerObserver(aplClientBridge);
    client->addCallStateObserver(m_guiManager);
    client->addFocusManagersObserver(m_guiManager);
    client->addAudioInputProcessorObserver(m_guiManager);
    m_guiManager->setClient(client);
    m_guiClient->setGUIManager(m_guiManager);
    
    m_shutdownManager = client->getShutdownManager();
    if (!m_shutdownManager) {
        TRACE(AVSClient, (_T("Failed to get ShutdownManager!")));
        return false;
    }

    // Thunder Input Manager
    m_thunderInputManager = ThunderInputManager::create(m_guiManager);
    if (!m_thunderInputManager) {
        TRACE(AVSClient, (_T("Failed to create m_thunderInputManager")));
      return false;
    }

    delAuth->addAuthObserver(m_guiClient);
    client->getRegistrationManager()->addObserver(m_guiClient);
    m_capabilitiesDelegate->addCapabilitiesObserver(m_guiClient);
    m_guiManager->setDoNotDisturbSettingObserver(m_guiClient);
    m_guiManager->configureSettingsNotifications();
    if (!m_guiClient->start()) {
        return false;
    }
    
    
    //delAuth->addAuthObserver(m_thunderInputManager);
    client->addMessageObserver(m_thunderInputManager);
    client->addAlexaDialogStateObserver(m_thunderInputManager);
    client->addAudioPlayerObserver(m_thunderInputManager);

    // skillmapper
    // since smartscreen sdk is just initialized pass audioPlayer state as false(not playing).
    vta.handleSDKStateChangeNotification(skillmapper::VoiceSDKState::VTA_INIT, true, false);

    client->connect();
    return true;
    }

      
    bool SmartScreen::InitSDKLogs(const string& logLevel)
    {
//#if 1
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
#if 0
        if (status == true) {
            apl::LoggerFactory::instance().initialize(std::make_shared<alexaSmartScreenSDK::sampleApp::AplCoreEngineSDKLogBridge>(alexaSmartScreenSDK::sampleApp::AplCoreEngineSDKLogBridge()));
        }

        return status;
    #endif
    return true;
    }

    bool SmartScreen::JsonConfigToStream(std::vector<std::shared_ptr<std::istream>>& streams, const std::string& configFile)
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

    bool SmartScreen::Deinitialize()
    {
        TRACE_L1(_T("Deinitialize()"));

        return true;
    }

    WPEFramework::Exchange::IAVSController* SmartScreen::Controller()
    {
        // Smart screen is controlled by websocket API and Controller is not needed
        return nullptr;
    }

    void SmartScreen::StateChange(WPEFramework::PluginHost::IShell* audiosource)
    {
        if (m_thunderVoiceHandler) {
            m_thunderVoiceHandler->stateChange(audiosource);
        }
    }
}
}



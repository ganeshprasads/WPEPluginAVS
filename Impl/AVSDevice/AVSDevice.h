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

#pragma once
#include "TraceCategories.h"
#include "ThunderInputManager.h"
#include "ThunderVoiceHandler.h"

#include <WPEFramework/interfaces/IAVSClient.h>

#include <AVSCommon/AVS/Initialization/InitializationParametersBuilder.h>
#include <AVS/KWD/AbstractKeywordDetector.h>
#include <AVS/SampleApp/SampleApplication.h>

#include <AVSCommon/SDKInterfaces/Diagnostics/ProtocolTracerInterface.h>
#include <AVSCommon/SDKInterfaces/Endpoints/EndpointBuilderInterface.h>
#include <AVSCommon/Utils/Configuration/ConfigurationNode.h>
#include <AVSCommon/Utils/DeviceInfo.h>
#include <AVSCommon/Utils/LibcurlUtils/HTTPContentFetcherFactory.h>
#include <AVSCommon/Utils/Network/InternetConnectionMonitor.h>
#include <acsdkAlerts/Storage/SQLiteAlertStorage.h>

#include <acsdkManufactory/Manufactory.h>
#include <ACL/Transport/HTTP2TransportFactory.h>
#include <ACL/Transport/PostConnectSequencerFactory.h>
#include <AVSCommon/AVS/CapabilitySemantics/CapabilitySemantics.h>
#include <AVSCommon/AVS/Initialization/InitializationParametersBuilder.h>
#include <AVSCommon/SDKInterfaces/PowerResourceManagerInterface.h>
#include <AVSCommon/Utils/LibcurlUtils/LibcurlHTTP2ConnectionFactory.h>
#include <AVSCommon/Utils/UUIDGeneration/UUIDGeneration.h>
#include <AVSGatewayManager/AVSGatewayManager.h>
#include <AVSGatewayManager/Storage/AVSGatewayManagerStorage.h>
#include <SynchronizeStateSender/SynchronizeStateSenderFactory.h>
#include <AVS/SampleApp/ExternalCapabilitiesBuilder.h>
#include <AVSCommon/AVS/Initialization/AlexaClientSDKInit.h>
#include <AVSCommon/SDKInterfaces/ApplicationMediaInterfaces.h>
#include <AVSCommon/SDKInterfaces/ChannelVolumeInterface.h>
#include <AVSCommon/SDKInterfaces/Diagnostics/DiagnosticsInterface.h>
#include <AVSCommon/Utils/MediaPlayer/PooledMediaPlayerFactory.h>
#include <CapabilitiesDelegate/CapabilitiesDelegate.h>
#include <acsdkExternalMediaPlayer/ExternalMediaPlayer.h>
#include <AVS/SampleApp/SampleApplicationComponent.h>
#include <AVSCommon/SDKInterfaces/LocaleAssetsManagerInterface.h>
#include <AVS/SampleApp/GuiRenderer.h>
#include <AVS/SampleApp/SampleApplicationReturnCodes.h>
#include <AVS/SampleApp/UserInputManager.h>
#include <AVS/SampleApp/UIManager.h>
#include <MediaPlayer/MediaPlayer.h>

#include <acsdkNotifications/SQLiteNotificationsStorage.h>
#include <CBLAuthDelegate/CBLAuthDelegate.h>
#include <CBLAuthDelegate/SQLiteCBLAuthDelegateStorage.h>
#include <CapabilitiesDelegate/CapabilitiesDelegate.h>
#include <CapabilitiesDelegate/Storage/SQLiteCapabilitiesDelegateStorage.h>
#include <SQLiteStorage/SQLiteMiscStorage.h>
#include <SampleApp/CaptionPresenter.h>
#include <SampleApp/SampleEqualizerModeController.h>
#include <Settings/Storage/SQLiteDeviceSettingStorage.h>

#include <acsdkEqualizerImplementations/InMemoryEqualizerConfiguration.h>
#include <acsdkEqualizerImplementations/MiscDBEqualizerStorage.h>
#include <acsdkEqualizerImplementations/SDKConfigEqualizerConfiguration.h>
#include <acsdkEqualizerInterfaces/EqualizerInterface.h>
#include <InterruptModel/config/InterruptModelConfiguration.h>



#include <vector>
#include <VoiceToApps/VideoSkillInterface.h>

namespace WPEFramework {
namespace Plugin {

    class AVSDevice
        : public WPEFramework::Exchange::IAVSClient,
          public Core::Thread,
          private alexaClientSDK::sampleApp::SampleApplication {
    public:
        AVSDevice()
            : _service(nullptr)
            , m_thunderInputManager(nullptr)
            , m_thunderVoiceHandler(nullptr)
        {
           Run();
        }

        AVSDevice(const AVSDevice&) = delete;
        AVSDevice& operator=(const AVSDevice&) = delete;
        ~AVSDevice()
        {
            Stop();
            Wait(Thread::STOPPED | Thread::BLOCKED, Core::infinite);
        }
        void CreateSQSWorker(void)
        {
            std::cout << "Starting SQS Thread.." << std::endl;
            while((IsRunning() == true)){ handleReceiveSQSMessage(); }
            return;
        }
    private:
        virtual uint32_t Worker()
        {
            if ((IsRunning() == true)) {
                CreateSQSWorker();
            }
            Block();
            return (Core::infinite);
        }
        class Config : public WPEFramework::Core::JSON::Container {
        public:
            Config(const Config&) = delete;
            Config& operator=(const Config&) = delete;

        public:
            Config()
                : WPEFramework::Core::JSON::Container()
                , Audiosource()
                , AlexaClientConfig()
                , LogLevel()
                , KWDModelsPath()
                , EnableKWD()
            {
                Add(_T("audiosource"), &Audiosource);
                Add(_T("alexaclientconfig"), &AlexaClientConfig);
                Add(_T("loglevel"), &LogLevel);
                Add(_T("kwdmodelspath"), &KWDModelsPath);
                Add(_T("enablekwd"), &EnableKWD);
            }

            ~Config() = default;

        public:
            WPEFramework::Core::JSON::String Audiosource;
            WPEFramework::Core::JSON::String AlexaClientConfig;
            WPEFramework::Core::JSON::String LogLevel;
            WPEFramework::Core::JSON::String KWDModelsPath;
            WPEFramework::Core::JSON::Boolean EnableKWD;
        };

    public:
        bool Initialize(PluginHost::IShell* service, const string& configuration) override;
        bool Deinitialize() override;
        Exchange::IAVSController* Controller() override;
        void StateChange(PluginHost::IShell* audioSource) override;

        BEGIN_INTERFACE_MAP(AVSDevice)
        INTERFACE_ENTRY(WPEFramework::Exchange::IAVSClient)
        END_INTERFACE_MAP

    private:
        bool Init(const std::string& audiosource, const bool enableKWD, const std::string& pathToInputFolder, const std::string& alexaClientConfig);
        bool InitSDKLogs(const string& logLevel);
        bool JsonConfigToStream(std::vector<std::shared_ptr<std::istream>>& streams, const std::string& configFile);

    private:
        WPEFramework::PluginHost::IShell* _service;
        std::shared_ptr<ThunderInputManager> m_thunderInputManager;
        std::shared_ptr<ThunderVoiceHandler<alexaClientSDK::sampleApp::InteractionManager>> m_thunderVoiceHandler;
#if defined(KWD_PRYON)
        std::unique_ptr<alexaClientSDK::kwd::AbstractKeywordDetector> m_keywordDetector;
#endif




    };

}
}

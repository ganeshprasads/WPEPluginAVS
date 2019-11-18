#pragma once
// TODO: make sure it is needed here
#define ACSDK_LOG_SINK Thunder

#include "Module.h"

#include "AVSDevice/SampleApplication.h"
#include "AVSDevice/SampleApplicationReturnCodes.h"

#ifdef ENABLE_SMART_SCREEN
#include "SmartScreen/SampleApplication.h"
#include "SmartScreen/SampleApplicationReturnCodes.h"
#endif

#ifdef ENABLE_USER_INPUT_MANAGER
#include "ConsoleReader.h"
#endif


// TODO: ifdef appthread
namespace WPEFramework {
namespace Plugin {

    class AVS
        : public PluginHost::IPlugin,
          public PluginHost::JSONRPC {
    public:
        AVS(const AVS&) = delete;
        AVS& operator=(const AVS&) = delete;

    private:

        class Notification : public RPC::IRemoteConnection::INotification {
        public:
            Notification() = delete;
            Notification(const Notification&) = delete;

        public:
            explicit Notification(AVS* parent)
                : _parent(*parent)
            {
                ASSERT(parent != nullptr);
            }
            virtual ~Notification() = default;

        public:
            virtual void Activated(RPC::IRemoteConnection* connection) { _parent.Activated(connection); }

            virtual void Deactivated(RPC::IRemoteConnection* connection) { _parent.Deactivated(connection); }

            BEGIN_INTERFACE_MAP(Notification)
                INTERFACE_ENTRY(RPC::IRemoteConnection::INotification)
            END_INTERFACE_MAP

        private:
            AVS& _parent;
        };

        class AppThread : public Core::Thread {
        public:
            AppThread(const AppThread&) = delete;
            AppThread& operator=(const AppThread&) = delete;

            AppThread(AVS* parent)
                : _parent(parent)
            {
            }

            uint32_t Worker() override
            {
                while (IsRunning() == true) {
                    if (_parent) {
                        // TODO: This will hang until the keypress, thus stopping whole
                        // plugin deinitialization; Make proper InputManager
                        alexaClientSDK::sampleApp::SampleAppReturnCode rc = _parent->_AVSDeviceClient->run();
                        if (rc != alexaClientSDK::sampleApp::SampleAppReturnCode::OK) {
                            TRACE_L1(("AVSClient->run() failed (%d)", rc));
                            break;
                        }
                    }
                }
                return Core::infinite;
            }

        private:
            AVS* _parent;
        };

        class Config : public Core::JSON::Container {
        public:
            Config(const Config&) = delete;
            Config& operator=(const Config&) = delete;

        public:
            Config()
                : Core::JSON::Container()
                , Audiosource()
                , AlexaClientConfig()
                , SmartScreenConfig()
                , LogLevel()
                , PathToKWDInputDir()
                , EnableSmartScreen()
            {
                Add(_T("audiosource"), &Audiosource);
                Add(_T("alexaclientconfig"), &AlexaClientConfig);
                Add(_T("smartscreenconfig"), &SmartScreenConfig);
                Add(_T("loglevel"), &LogLevel);
                Add(_T("pathtokwdinputdir"), &PathToKWDInputDir);
                Add(_T("enablesmartscreen"), &EnableSmartScreen);
            }

            ~Config() = default;

        public:
            Core::JSON::String Audiosource;
            Core::JSON::String AlexaClientConfig;
            Core::JSON::String SmartScreenConfig;
            Core::JSON::String LogLevel;
            Core::JSON::String PathToKWDInputDir;
            Core::JSON::Boolean EnableSmartScreen;
        };


    public:
        AVS()
            : _service(nullptr)
            , _notification(this)
            , _connectionId(0)
            , _appThread(this)
            , _AVSDeviceClient()
#ifdef ENABLE_SMART_SCREEN
            , _SmartScreenClient()
#endif
            , _consoleReader()
            , _configFiles()
        {
            RegisterAll();
        }

        virtual ~AVS()
        {
            UnregisterAll();
        }

        BEGIN_INTERFACE_MAP(AVS)
            INTERFACE_ENTRY(PluginHost::IPlugin)
            INTERFACE_ENTRY(PluginHost::IDispatcher)
        END_INTERFACE_MAP

        //   IPlugin methods
        // -------------------------------------------------------------------------------------------------------
        const string Initialize(PluginHost::IShell* service) override;
        void Deinitialize(PluginHost::IShell* service) override;
        string Information() const override;

    private:
        void Activated(RPC::IRemoteConnection* connection);
        void Deactivated(RPC::IRemoteConnection* connection);

        //   JSONRPC methods
        // -------------------------------------------------------------------------------------------------------
        void RegisterAll();
        void UnregisterAll();
        uint32_t endpoint_tap();

        PluginHost::IShell* _service;
        Core::Sink<Notification> _notification;
        uint32_t _connectionId;

        AppThread _appThread;
        std::unique_ptr<alexaClientSDK::sampleApp::SampleApplication> _AVSDeviceClient;
#ifdef ENABLE_SMART_SCREEN
        std::unique_ptr<alexaSmartScreenSDK::sampleApp::SampleApplication> _SmartScreenClient;
#endif
        std::shared_ptr<alexaClientSDK::sampleApp::ConsoleReader> _consoleReader;

        // TODO: make a string or a JSON config
        std::vector<std::string> _configFiles;
    };

} // namespace Plugin
} // namespace WPEFramework

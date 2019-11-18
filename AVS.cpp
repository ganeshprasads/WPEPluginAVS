#include "AVS.h"

namespace WPEFramework {
namespace Plugin {
    SERVICE_REGISTRATION(AVS, 1, 0);

    const string AVS::Initialize(PluginHost::IShell* service)
    {
        string message = EMPTY_STRING;
        Config config;

        ASSERT(service != nullptr);
        ASSERT(_service == nullptr);

        _service = service;
        _service->Register(&_notification);

        config.FromString(_service->ConfigLine());
        if (!config.AlexaClientConfig.IsSet()) {
            message = "Missing AlexaClient config file";
            TRACE_L1(message);
            return message;
        }

        if (!config.LogLevel.IsSet()) {
            message = "Missing log level";
            TRACE_L1(message);
            return message;
        }

        if(config.Audiosource.Value().empty()) {
            message = "Missign audiosource callsing";
            TRACE_L1(message);
            return message;
        }

        if(config.EnableSmartScreen.Value() == true)
        {
#if defined(ENABLE_SMART_SCREEN)
            TRACE_L1("Launching Smart Screen Client...")
            fprintf(stderr, "*** SMART SCREEN ON\n");
            _SmartScreenClient = alexaSmartScreenSDK::sampleApp::SampleApplication::create(
                service,
                config.AlexaClientConfig.Value(),
                config.SmartScreenConfig.Value(),
                config.PathToKWDInputDir.Value(),
                config.Audiosource.Value(),
                config.LogLevel.Value());

            if (!_SmartScreenClient) {
                message = "Failed to create AVSClient";
                return message;
            }
#else
            message = "Smart Screen Support is not compiled in!";
            TRACE_L1(message);
            return message;
#endif
        } else {
            TRACE_L1("Launching AVSDevice Client...")
// TODO: clean input manager
#ifdef ENABLE_USER_INPUT_MANAGER
            auto _consoleReader = std::make_shared<alexaClientSDK::sampleApp::ConsoleReader>();
            if (!_consoleReader) {
                message = "Failed to create console reader";
                return message;
            }

            _AVSDeviceClient = alexaClientSDK::sampleApp::SampleApplication::create(
                _service,
                _consoleReader,
                config.AlexaClientConfig.Value(),
                config.SmartScreenConfig.Value(),
                config.PathToKWDInputDir.Value(),
                config.Audiosource.Value(),
                config.LogLevel.Value());

            if (!_AVSDeviceClient) {
                message = "Failed to create AVSClient";
                return message;
            }
            _appThread.Run();
#endif

// TODO: remove that shit
#ifdef ENABLE_THUNDER_INPUT_MANAGER
            _AVSDeviceClient = alexaClientSDK::sampleApp::SampleApplication::create(
                _service,
                nullptr,
                config.AlexaClientConfig.Value(),
                config.SmartScreenConfig.Value(),
                config.PathToKWDInputDir.Value(),
                config.Audiosource.Value(),
                config.LogLevel.Value());

            if (!_AVSDeviceClient) {
                message = "Failed to create AVSClient";
                return message;
            }
#endif
        }
        return message;
    }

    void AVS::Deinitialize(PluginHost::IShell* service)
    {
        ASSERT(_service == service);

        TRACE_L1("Stopping notifications...");
        _service->Unregister(&_notification);
        _service = nullptr;
#ifdef ENABLE_USER_INPUT_MANAGER
        TRACE_L1("Stopping console reader...");
        _consoleReader.reset();
        TRACE_L1("Stopping application thread...");
        _appThread.Stop();
        TRACE_L1("Waiting for application thread close...");
        _appThread.Wait(Core::Thread::STOPPED | Core::Thread::BLOCKED, Core::infinite);
#endif
        TRACE_L1("Stopping AVSClient...");
        // TODO: _AVSDeviceClient will not be release due to infinite wait for input
        _AVSDeviceClient.reset();
        TRACE_L1("All done");
    }

    string AVS::Information() const
    {
        return ((_T("Alexa Voice Service Headless Client")));
    }

    void AVS::Activated(RPC::IRemoteConnection* /*connection*/)
    {
        return;
    }

    void AVS::Deactivated(RPC::IRemoteConnection* connection)
    {
        if (_connectionId == connection->Id()) {
            ASSERT(_service != nullptr);
            PluginHost::WorkerPool::Instance().Submit(PluginHost::IShell::Job::Create(_service, PluginHost::IShell::DEACTIVATED, PluginHost::IShell::FAILURE));
        }
    }
} // namespace Plugin
} // namespace WPEFramework

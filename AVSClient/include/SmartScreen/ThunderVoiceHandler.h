
/*
 * Copyright 2017-2019 Amazon.com, Inc. or its affiliates. All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License").
 * You may not use this file except in compliance with the License.
 * A copy of the License is located at
 *
 *     http://aws.amazon.com/apache2.0/
 *
 * or in the "license" file accompanying this file. This file is distributed
 * on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either
 * express or implied. See the License for the specific language governing
 * permissions and limitations under the License.
 */

#pragma once

#include <mutex>
#include <thread>

#include <AVSCommon/AVS/AudioInputStream.h>
#include <Audio/MicrophoneInterface.h>

#include "GUI/GUIManager.h"

#include <WPEFramework/plugins/plugins.h>
#include <WPEFramework/interfaces/IVoiceHandler.h>

namespace alexaSmartScreenSDK {
namespace sampleApp {

struct IInteractionHandler {
    virtual ~IInteractionHandler() = default;
    virtual bool Initialize(std::shared_ptr<alexaSmartScreenSDK::sampleApp::gui::GUIManager> interactionManager) = 0;
    virtual bool Deinitialize() = 0;
    virtual std::shared_ptr<alexaSmartScreenSDK::sampleApp::gui::GUIManager> Manager() const = 0;
};

/// This class provides and audio input from Thunder
class ThunderVoiceHandler : public alexaClientSDK::applicationUtilities::resources::audio::MicrophoneInterface {
public:
    /**
     * Creates a @c ThunderVoiceHandler.
     *
     * @param stream The shared data stream to write to.
     * @param service The Thunder PluginHost::IShell of the nano service.
     * @param callsign The callsign of nano service providing audio input.
     * @param interactionHandler The interaction handler that will execute necessary action like holdToTalk.
     * @return A unique_ptr to a @c ThunderVoiceHandler if creation was successful and @c nullptr otherwise.
     */
    static std::unique_ptr<ThunderVoiceHandler> create(std::shared_ptr<alexaClientSDK::avsCommon::avs::AudioInputStream> stream, WPEFramework::PluginHost::IShell *service, const string& callsign, std::shared_ptr<IInteractionHandler> interactionHandler);

    /**
     * Stops streaming from the microphone.
     *
     * @return Whether the stop was successful.
     */
    bool stopStreamingMicrophoneData() override;

    /**
     * Starts streaming from the microphone.
     *
     * @return Whether the start was successful.
     */
    bool startStreamingMicrophoneData() override;

    /**
     * Destructor.
     */
    ~ThunderVoiceHandler();

private:
    /**
     * Constructor.
     *
     * @param stream The shared data stream to write to.
     * @param service The Thunder PluginHost::IShell of the nano service.
     * @param callsign The callsign of nano service providing audio input.
     * @param interactionHandler The interaction handler that will execute necessary action like holdToTalk.
     */
    ThunderVoiceHandler(std::shared_ptr<alexaClientSDK::avsCommon::avs::AudioInputStream> stream, WPEFramework::PluginHost::IShell *service, const string& callsign, std::shared_ptr<IInteractionHandler> interactionHandler);

    /// Initializes ThunderVoiceHandler.
    bool initialize();

public:
    /// Responsible for making an interaction on audio data
    class InteractionHandler : public IInteractionHandler {
    public:
        /**
         * Creates a @c InteractionHandler.
         * After creation this class must be initialized with proper Interaction Manager
         *
         * @return A unique_ptr to a @c InteractionHandler if creation was successful and @c nullptr otherwise.
         */
        static std::unique_ptr<InteractionHandler> Create();

        InteractionHandler(const InteractionHandler&) = delete;
        InteractionHandler& operator=(const InteractionHandler&) = delete;
        ~InteractionHandler() = default;

    private:
        InteractionHandler();

    public:
        bool Initialize(std::shared_ptr<alexaSmartScreenSDK::sampleApp::gui::GUIManager> interactionManager) override;
        bool Deinitialize() override;
        std::shared_ptr<alexaSmartScreenSDK::sampleApp::gui::GUIManager> Manager() const override;

    private:
        std::shared_ptr<alexaSmartScreenSDK::sampleApp::gui::GUIManager> m_interactionManager;

    };

private:
    ///  Responsible for getting audio data from Thunder
    class VoiceHandler : public WPEFramework::Exchange::IVoiceHandler
    {
    public:
        /**
         * Contructor
         *
         * @param parent The parent class
         *
         */
        VoiceHandler(ThunderVoiceHandler *parent);

        /**
         * A callback method that is invoked with each start of the audio transmission.
         * The profile should stay the same between @c Start() and @c Stop().
         *
         * @param profile The audio profile of incoming data.
         */
        void Start(const WPEFramework::Exchange::IVoiceProducer::IProfile* profile) override;

        /**
         * A callback method that is invoked with each stop of the audio transmission.
         * The profile after @c Stop() is no longer valid.
         *
         */
        void Stop() override;

        /**
         * A callback method that is invoked with each data of the audio transmission.
         *
         * @param sequenceNo The sequence number of incoming data.
         * @param data The data itself.
         * @param length The length in bytes of audio data.
         */
        void Data(const uint32_t sequenceNo, const uint8_t data[], const uint16_t length) override;

        // TODO: get rid of using namespace WPEFramework in header!
        BEGIN_INTERFACE_MAP(VoiceHandler)
            INTERFACE_ENTRY(WPEFramework::Exchange::IVoiceHandler)
        END_INTERFACE_MAP

    private:
        /// Holds the profile between @c Start() and @c Stop().
        const WPEFramework::Exchange::IVoiceProducer::IProfile* m_profile;
        /// The parent class.
        ThunderVoiceHandler *m_parent;
    };

    /// Responsible for monitoring if a callsign has been activated
    class CallsignNotification : public WPEFramework::PluginHost::IPlugin::INotification
    {
    public:
      CallsignNotification(const CallsignNotification&) = delete;
      CallsignNotification& operator=(const CallsignNotification&) = delete;

    public:
        /**
         * Constructor
         */
        explicit CallsignNotification(ThunderVoiceHandler* parent);

        /**
         * Destructor
         */
        ~CallsignNotification() = default;

        BEGIN_INTERFACE_MAP(PluginNotification)
            INTERFACE_ENTRY(WPEFramework::PluginHost::IPlugin::INotification)
        END_INTERFACE_MAP

    public:
        // WPEFramework::PluginHost::IPlugin::INotification implementation
        /**
         * Callback indicating a plugin state change.
         *
         * @param plugin Holds information of the plugin that is in a state change.
         */
        void StateChange(WPEFramework::PluginHost::IShell* plugin) override;

    private:
        /// The parent class.
        ThunderVoiceHandler* m_parent;
    };

private:
    /// The stream of audio data.
    const std::shared_ptr<alexaClientSDK::avsCommon::avs::AudioInputStream> m_audioInputStream;

    /// The writer that will be used to writer audio data into the sds.
    std::shared_ptr<alexaClientSDK::avsCommon::avs::AudioInputStream::Writer> m_writer;

    /**
     * A lock to seralize access to startStreamingMicrophoneData() and stopStreamingMicrophoneData() between different
     * threads.
     */
    std::mutex m_mutex;

    /// The notification sink for @c CallsingNotification.
    WPEFramework::Core::Sink<CallsignNotification> m_callsignNotification;
    /// The callsing of the plugin providin voice audio input.
    string m_callsign;
    /// The Thunder PluginHost::IShell of the AVS plugin.
    WPEFramework::PluginHost::IShell* m_service;
    /// The Voice producer implementation
    WPEFramework::Exchange::IVoiceProducer* m_voiceProducer;
    /// The Voice Handler implementation
    WPEFramework::Core::ProxyType<VoiceHandler> m_voiceHandler;
    std::shared_ptr<IInteractionHandler> m_interactionHandler;

};

}  // namespace sampleApp
}  // namespace alexaClientSDK

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

#include "ThunderVoiceHandler.h"

#include <cstring>
#include <string>

#include <rapidjson/document.h>

#include <AVSCommon/Utils/Configuration/ConfigurationNode.h>
#include <AVSCommon/Utils/Logger/Logger.h>

namespace alexaSmartScreenSDK {
namespace sampleApp {

using alexaClientSDK::avsCommon::avs::AudioInputStream;
using namespace WPEFramework;
using namespace alexaSmartScreenSDK::sampleApp::gui;

/// String to identify log entries originating from this file.
static const std::string TAG("ThunderVoiceHandler");

// TODO: sync with the audio profile
// supported word size in bits
static constexpr uint8_t SUPPORTED_WORD_SIZE = 16;

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

std::unique_ptr<ThunderVoiceHandler> ThunderVoiceHandler::create(
    std::shared_ptr<AudioInputStream> stream,
    PluginHost::IShell *service,
    const string& callsign,
    std::shared_ptr<IInteractionHandler> interactionHandler) {

    if (!stream) {
        ACSDK_CRITICAL(LX("Invalid stream"));
        return nullptr;
    }

    if(!service) {
        ACSDK_CRITICAL(LX("Invalid service"));
        return nullptr;
    }

    std::unique_ptr<ThunderVoiceHandler> thunderVoiceHandler(new ThunderVoiceHandler(stream, service, callsign, interactionHandler));
    if(!thunderVoiceHandler) {
        ACSDK_CRITICAL(LX("Failed to create a ThunderVoiceHandler!"));
        return nullptr;
    }

    // try to initialze thunder voice handler
    // if it fails subscribe for a notification that will do initialization
    if(!thunderVoiceHandler->initialize()) {
        ACSDK_DEBUG0(LX("ThunderVoiceHandler is not initialized. Subscribing for notification..."));
        service->Register(&thunderVoiceHandler->m_callsignNotification);
    }

    return thunderVoiceHandler;
}

bool ThunderVoiceHandler::startStreamingMicrophoneData() {
    ACSDK_INFO(LX(__func__));
    std::lock_guard<std::mutex> lock{m_mutex};

    return true;
}

bool ThunderVoiceHandler::stopStreamingMicrophoneData() {
    ACSDK_INFO(LX(__func__));
    std::lock_guard<std::mutex> lock{m_mutex};

    return true;
}

ThunderVoiceHandler::~ThunderVoiceHandler() {
    // TODO: clean up
}

ThunderVoiceHandler::ThunderVoiceHandler(std::shared_ptr<AudioInputStream> stream, PluginHost::IShell *service, const string& callsign, std::shared_ptr<IInteractionHandler> interactionHandler)
        : m_audioInputStream{stream}
        , m_callsignNotification{this}
        , m_callsign{callsign}
        , m_service{service}
        , m_voiceProducer{nullptr}
        , m_voiceHandler{Core::ProxyType<VoiceHandler>::Create(this)}
        , m_interactionHandler{interactionHandler}
{
}

bool ThunderVoiceHandler::initialize() {
    m_writer = m_audioInputStream->createWriter(AudioInputStream::Writer::Policy::NONBLOCKABLE);
    if (!m_writer) {
        ACSDK_CRITICAL(LX("Failed to create stream writer"));
        return false;
    }

    m_voiceProducer = m_service->QueryInterfaceByCallsign<Exchange::IVoiceProducer>(m_callsign);
    if(!m_voiceProducer) {
        ACSDK_ERROR(LX("Failed to obtain VoiceProducer interface!"));
        return false;
    }

    if(!m_voiceHandler) {
        ACSDK_ERROR(LX("Failed to obtain VoiceHandler!"));
        return false;
    }

    m_voiceProducer->Callback((&(*m_voiceHandler)));

    return true;
}

std::unique_ptr<ThunderVoiceHandler::InteractionHandler> ThunderVoiceHandler::InteractionHandler::Create()
{
    std::unique_ptr<InteractionHandler> interactionHandler(new InteractionHandler());
    if(!interactionHandler) {
        ACSDK_CRITICAL(LX("Failed to create a interaction handler!"));
        return nullptr;
    }

    return interactionHandler;
}

ThunderVoiceHandler::InteractionHandler::InteractionHandler()
    : m_interactionManager{nullptr}
{

}

bool ThunderVoiceHandler::InteractionHandler::Initialize(std::shared_ptr<GUIManager> interactionManager)
{
    bool status = true;
    if(interactionManager) {
        m_interactionManager = interactionManager;
        status = true;
    }
    return status;
}

bool ThunderVoiceHandler::InteractionHandler::Deinitialize()
{
    bool status = true;
    if(m_interactionManager) {
        m_interactionManager.reset();
    }
    return status;
}

std::shared_ptr<GUIManager> ThunderVoiceHandler::InteractionHandler::Manager() const
{
    return m_interactionManager;
}

ThunderVoiceHandler::VoiceHandler::VoiceHandler(ThunderVoiceHandler *parent)
    : m_profile{nullptr}
    , m_parent{parent}
{

}

void ThunderVoiceHandler::VoiceHandler::Start(const Exchange::IVoiceProducer::IProfile* profile)
{
    ACSDK_DEBUG0(LX("ThunderVoiceHandler::VoiceHandler::Start()"));

    m_profile = profile;
    if(m_profile) {
        m_profile->AddRef();
    }

    if(m_parent && m_parent->m_interactionHandler) {
        std::shared_ptr<GUIManager> manager = m_parent->m_interactionHandler->Manager();
        if(manager) {
            manager->handleHoldToTalk();
        }
    }
}

void ThunderVoiceHandler::VoiceHandler::Stop()
{
    ACSDK_DEBUG0(LX("ThunderVoiceHandler::VoiceHandler::Stop()"));

    if(m_profile){
        m_profile->Release();
        m_profile = nullptr;
    }

    if(m_parent && m_parent->m_interactionHandler) {
        std::shared_ptr<GUIManager> manager = m_parent->m_interactionHandler->Manager();
        if(manager) {
            manager->handleHoldToTalk();
        }
    }
}

void ThunderVoiceHandler::VoiceHandler::Data(const uint32_t sequenceNo, const uint8_t data[] /* @length:length */, const uint16_t length)
{
    ACSDK_DEBUG0(LX("ThunderVoiceHandler::VoiceHandler::Data()"));

    if(m_parent) {
        // incoming data length = number of bytes
        size_t nWords = length / m_parent->m_writer->getWordSize();
        ssize_t rc = m_parent->m_writer->write(data, nWords);
        if(rc <= 0) {
            ACSDK_CRITICAL(LX("Failed to write to stream.").d("rc", rc));
        }
    }
}

ThunderVoiceHandler::CallsignNotification::CallsignNotification(ThunderVoiceHandler* parent)
    : m_parent{parent}
{

}

void ThunderVoiceHandler::CallsignNotification::StateChange(PluginHost::IShell* service)
{
    if(!service) {
        ACSDK_INFO(LX("Service is a nullptr!"));
        return;
    }

    if(service->Callsign() == m_parent->m_callsign) {
        if(service->State() == PluginHost::IShell::ACTIVATED) {
            bool status = m_parent->initialize();
            if(!status) {
                ACSDK_CRITICAL(LX("Failed to initialize ThunderVoiceHandlerWraper"));
                return;
            }
        }
    }
}

}  // namespace sampleApp
}  // namespace alexaClientSDK

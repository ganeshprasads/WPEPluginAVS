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

#include "ThunderInputManager.h"

namespace alexaClientSDK {
namespace sampleApp {

    using namespace avsCommon::sdkInterfaces;

    /// String to identify log entries originating from this file.
    static const std::string TAG("ThunderInputManager");

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

    std::unique_ptr<ThunderInputManager> ThunderInputManager::create(
        std::shared_ptr<InteractionManager> interactionManager)
    {
        if (!interactionManager) {
            ACSDK_CRITICAL(LX("Invalid InteractionManager passed to ThunderInputManager"));
            return nullptr;
        }

        return std::unique_ptr<ThunderInputManager>(new ThunderInputManager(interactionManager));
    }

    ThunderInputManager::ThunderInputManager(
        std::shared_ptr<InteractionManager> interactionManager)
        : m_interactionManager{ interactionManager }
        , m_limitedInteraction{ false }
    {
    }

    SampleAppReturnCode ThunderInputManager::exec(InputCommand command)
    {
        SampleAppReturnCode status = SampleAppReturnCode::OK;
        if (m_limitedInteraction) {
            status = SampleAppReturnCode::BUSY;
        } else {
            m_limitedInteraction = true;
            switch (command) {
            case InputCommand::TAP:
                m_interactionManager->tap();
                break;
            default:
                status = SampleAppReturnCode::ERROR;
                break;
            }
            m_limitedInteraction = false;
        }
        return status;
    }
    void ThunderInputManager::onLogout()
    {
        m_limitedInteraction = true;
    }

    void ThunderInputManager::onAuthStateChange(AuthObserverInterface::State newState, AuthObserverInterface::Error newError)
    {
        m_limitedInteraction = m_limitedInteraction || (newState == AuthObserverInterface::State::UNRECOVERABLE_ERROR);
    }

    void ThunderInputManager::onCapabilitiesStateChange(
        CapabilitiesObserverInterface::State newState,
        CapabilitiesObserverInterface::Error newError)
    {
        m_limitedInteraction = m_limitedInteraction || (newState == CapabilitiesObserverInterface::State::FATAL_ERROR);
    }
} // namespace sampleApp
} // namespace alexaClientSDK

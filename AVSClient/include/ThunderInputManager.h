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

#include <atomic>

#include "InteractionManager.h"
#include "SampleApplicationReturnCodes.h"

namespace alexaClientSDK {
namespace sampleApp {

    enum class InputCommand : uint8_t {
        TAP
    };

    /// Observes user input from the console and notifies the interaction manager of the user's intentions.
    class ThunderInputManager
        : public avsCommon::sdkInterfaces::AuthObserverInterface,
          public avsCommon::sdkInterfaces::CapabilitiesObserverInterface,
          public registrationManager::RegistrationObserverInterface {
    public:
        /**
     * Create a ThunderInputManager.
     *
     * @param interactionManager An instance of the @c InteractionManager used to manage user input.
     * @param localeAssetsManager The @c LocaleAssetsManagerInterface that provides the supported locales.
     * @return Returns a new @c ThunderInputManager, or @c nullptr if the operation failed.
     */
        static std::unique_ptr<ThunderInputManager> create(
            std::shared_ptr<InteractionManager> interactionManager);

        /**
     *  Executes a user command
     *  @param commmand a command to process
     *  @return Returns a @c SampleAppReturnCode.
     */
        SampleAppReturnCode exec(InputCommand command);

        /// @name RegistrationObserverInterface Functions
        /// @{
        void onLogout() override;
        /// @
    private:
        /**
     * Constructor.
     *
     * @param interactionManager An instance of the @c InteractionManager used to manage user input.
     * @param localeAssetsManager The @c LocaleAssetsManagerInterface that provides the supported locales.
     */
        ThunderInputManager(std::shared_ptr<InteractionManager> interactionManager);

        /// The main interaction manager that interfaces with the SDK.
        std::shared_ptr<InteractionManager> m_interactionManager;

        /// Flag to indicate that a fatal failure occurred. In this case, customer can either reset the device or kill
        /// the app.
        std::atomic_bool m_limitedInteraction;

        /// @name AuthObserverInterface Function
        /// @{
        void onAuthStateChange(AuthObserverInterface::State newState, AuthObserverInterface::Error newError) override;
        /// @}

        /// @name CapabilitiesObserverInterface Function
        /// @{
        void onCapabilitiesStateChange(
            CapabilitiesObserverInterface::State newState,
            CapabilitiesObserverInterface::Error newError) override;
        /// @
    };

} // namespace sampleApp
} // namespace alexaClientSDK

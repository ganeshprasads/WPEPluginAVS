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

#include "ThunderInputManager.h"

namespace WPEFramework {
namespace Plugin {

    using namespace alexaClientSDK::avsCommon::sdkInterfaces;
    using namespace WPEFramework::Exchange;

 #if defined(ENABLE_SMART_SCREEN_SUPPORT)
 
     std::unique_ptr<ThunderInputManager>
 ThunderInputManager::create(std::shared_ptr<alexaSmartScreenSDK::sampleApp::gui::GUIManager> guiManager)
     {
             if (!guiManager) {
                     TRACE_GLOBAL(AVSClient, (_T("Invalid guiManager passed to ThunderInputManager")));
                     return nullptr;
                 }
                 return std::unique_ptr<ThunderInputManager>(new ThunderInputManager(guiManager));
             }
             
             ThunderInputManager::ThunderInputManager(std::shared_ptr<alexaSmartScreenSDK::sampleApp::gui::GUIManager>
         guiManager)
                 : m_limitedInteraction{ false }
                , m_playerState{alexaClientSDK::avsCommon::avs::PlayerActivity::IDLE}
                 , m_interactionManager{ nullptr }
                 , m_guiManager{ nullptr }
                 , m_controller{ WPEFramework::Core::ProxyType<AVSController>::Create(this) }
             {
                     TRACE_L1("Parsing VoiceToApps LEDs...");
                     m_vtaFlag = vta.ioParse();
                 }
                 
 #endif


    std::unique_ptr<ThunderInputManager> ThunderInputManager::create(std::shared_ptr<alexaClientSDK::sampleApp::InteractionManager> interactionManager)
    {
        if (!interactionManager) {
            TRACE_GLOBAL(AVSClient, (_T("Invalid InteractionManager passed to ThunderInputManager")));
            return nullptr;
        }
        return std::unique_ptr<ThunderInputManager>(new ThunderInputManager(interactionManager));
    }

    ThunderInputManager::ThunderInputManager(std::shared_ptr<alexaClientSDK::sampleApp::InteractionManager> interactionManager)
        : m_limitedInteraction{ false }
        , m_interactionManager{ interactionManager }
        , m_controller{ WPEFramework::Core::ProxyType<AVSController>::Create(this) }
        , m_playerState{alexaClientSDK::avsCommon::avs::PlayerActivity::IDLE}
    {
        TRACE_L1("Parsing VoiceToApps LEDs...");
        m_vtaFlag = vta.ioParse();
    }

       // to check if audioPlayer is in playing/buffering/paused state
   bool ThunderInputManager::isAudioPlaying(void)
   {
       using namespace alexaClientSDK::avsCommon::avs;
       bool state=(m_playerState == PlayerActivity::PLAYING) || 
           (m_playerState == PlayerActivity::BUFFER_UNDERRUN) ||
           (m_playerState == PlayerActivity::PAUSED);
                std::cout << __func__ << "Current Aud State =" << state <<std::endl;
                return state;
   }
  
   void ThunderInputManager::onPlayerActivityChanged (alexaClientSDK::avsCommon::avs::PlayerActivity state, const Context &context) 

       {
                m_playerState = state; 
       std::cout << " onPlayerActivityChanged:  State change called ";
      using namespace skillmapper;
           using namespace alexaClientSDK::avsCommon::avs;
       AudioPlayerState smState;
       switch (state) {
       case PlayerActivity::IDLE:
           std::cout << "idle ";
           smState = AudioPlayerState::IDLE; 
           break;
       case PlayerActivity::PLAYING:
           std::cout << "playing ";
           smState = AudioPlayerState::PLAYING; 
           break;
       case PlayerActivity::STOPPED:
          std::cout << "stopped" ;
           smState = AudioPlayerState::STOPPED; 
           break;
       case PlayerActivity::PAUSED:
           std::cout << "paused ";
           smState = AudioPlayerState::PAUSED; 
           break;
       case PlayerActivity::BUFFER_UNDERRUN:
           std::cout << "bufferring ";
           smState = AudioPlayerState::BUFFER_UNDERRUN; 
           break;
       case PlayerActivity::FINISHED:
           std::cout << "finished ";
           smState = AudioPlayerState::FINISHED; 
           break;
       default:
           std::cout << "unknown ";
           smState = AudioPlayerState::UNKNOWN; 
       }

       std::cout << std::endl;
       vta.handleAudioPlayerStateChangeNotification(smState);
   }


    void ThunderInputManager::onDialogUXStateChanged(DialogUXState newState)
    {
        if (m_controller) {
            m_controller->NotifyDialogUXStateChanged(newState);
        }
    }
	
	void ThunderInputManager::renderTemplateCard(const std::string& jsonPayload, alexaClientSDK::avsCommon::avs::FocusState focusState)
    {
        if(! m_vtaFlag ) {
         TRACE_L1("VoiceToApps vtaFlag not initialized...");
          printf("VoiceToApps vtaFlag not initialized...\n");
          return;
        }

        printf("VoiceToApps template card: %s ...\n", jsonPayload.c_str());
        vta.curlCmdSendOnRcvMsg(jsonPayload);
    }

    void  ThunderInputManager::renderPlayerInfoCard (const std::string &jsonPayload, 
                TemplateRuntimeObserverInterface::AudioPlayerInfo info, alexaClientSDK::avsCommon::avs::FocusState focusState) 
    {

    }

    void ThunderInputManager::clearTemplateCard() {
            
    }

    void ThunderInputManager::clearPlayerInfoCard() {

    }



    void ThunderInputManager::receive(const std::string& contextId, const std::string& message) {
        TRACE_L1 ( "Message received from observer.. \n");

        if(! m_vtaFlag ) {
            TRACE_L1("VoiceToApps vtaFlag not initialized...");
            return;
        }

        std::string key = "\"namespace\":\"SpeechSynthesizer\",\"name\":\"Speak\"";
        if(std::string::npos!=message.find(key,0) )
            vta.curlCmdSendOnRcvMsg(message);
    }

    ThunderInputManager::AVSController::AVSController(ThunderInputManager* parent)
        : m_parent(*parent)
        , m_notifications()
    {
    }

    ThunderInputManager::AVSController::~AVSController()
    {
        for (auto* notification : m_notifications) {
            notification->Release();
        }
        m_notifications.clear();
    }

    void ThunderInputManager::AVSController::NotifyDialogUXStateChanged(DialogUXState newState)
    {
        IAVSController::INotification::dialoguestate dialoguestate;
        bool isStateHandled = true;
        using namespace skillmapper;
        bool smartScreenEnabled = 0;
        //bool (*audStateCb)(void)= &ThunderInputManager::isAudioPlaying;
       
#if defined(ENABLE_SMART_SCREEN_SUPPORT)
        smartScreenEnabled = 1;
#endif
        if (! m_parent.m_vtaFlag ) {
            TRACE_L1("VoiceToApps vtaFlag not initialized during state change...");
        }
    
   
        switch (newState) {
        case DialogUXState::IDLE:
            m_parent.vta.handleSDKStateChangeNotification(VoiceSDKState::VTA_IDLE, smartScreenEnabled,m_parent.isAudioPlaying() );
            std::cout << "Calling Smart screen notification with idle status " << std::endl;
            dialoguestate = IAVSController::INotification::IDLE;
            break;
        case DialogUXState::LISTENING:
            m_parent.vta.handleSDKStateChangeNotification(VoiceSDKState::VTA_LISTENING, smartScreenEnabled,m_parent.isAudioPlaying());
            dialoguestate = IAVSController::INotification::LISTENING;
            break;
        case DialogUXState::EXPECTING:
            m_parent.vta.handleSDKStateChangeNotification(VoiceSDKState::VTA_EXPECTING, smartScreenEnabled, m_parent.isAudioPlaying());
#ifdef FILEAUDIO
            if ( m_parent.vta.invocationMode ){
                 m_parent.vta.fromExpecting=true;
                 m_parent.vta.skipMerge=true;
            }
#endif
            dialoguestate = IAVSController::INotification::EXPECTING;
            break;
        case DialogUXState::THINKING:
            m_parent.vta.handleSDKStateChangeNotification(VoiceSDKState::VTA_THINKING, smartScreenEnabled, m_parent.isAudioPlaying() );
            dialoguestate = IAVSController::INotification::THINKING;
            break;
        case DialogUXState::SPEAKING:
            m_parent.vta.handleSDKStateChangeNotification(VoiceSDKState::VTA_SPEAKING, smartScreenEnabled,m_parent.isAudioPlaying() );
#ifdef FILEAUDIO
            if ( m_parent.vta.fromExpecting ) {
                 m_parent.vta.fromExpecting=false;
                 m_parent.vta.skipMerge=false;
            }
#endif
            dialoguestate = IAVSController::INotification::SPEAKING;
            break;
        case DialogUXState::FINISHED:
            TRACE(AVSClient, (_T("Unmapped Dialog state (%d)"), newState));
            isStateHandled = false;
            break;
        default:
            TRACE(AVSClient, (_T("Unknown State (%d)"), newState));
            isStateHandled = false;
        }

        if (isStateHandled == true) {
            for (auto* notification : m_notifications) {
                notification->DialogueStateChange(dialoguestate);
            }
        }
    }

    void ThunderInputManager::AVSController::Register(INotification* notification)
    {
        ASSERT(notification != nullptr);
        notification->AddRef();
        m_notifications.push_back(notification);
    }

    void ThunderInputManager::AVSController::Unregister(const INotification* notification)
    {
        ASSERT(notification != nullptr);

        auto item = std::find(m_notifications.begin(), m_notifications.end(), notification);
        if (item != m_notifications.end()) {
            m_notifications.erase(item);
            (*item)->Release();
        }
    }

    uint32_t ThunderInputManager::AVSController::Mute(const bool mute)
    {
        if (m_parent.m_limitedInteraction) {
            return static_cast<uint32_t>(WPEFramework::Core::ERROR_GENERAL);
        }

        if (!m_parent.m_interactionManager) {
            return static_cast<uint32_t>(WPEFramework::Core::ERROR_UNAVAILABLE);
        }

        m_parent.m_interactionManager->setMute(ChannelVolumeInterface::Type::AVS_SPEAKER_VOLUME, mute);
        m_parent.m_interactionManager->setMute(ChannelVolumeInterface::Type::AVS_ALERTS_VOLUME, mute);

        return static_cast<uint32_t>(WPEFramework::Core::ERROR_NONE);
    }

    uint32_t ThunderInputManager::AVSController::Record(const bool start)
    {
        if (m_parent.m_limitedInteraction) {
            return static_cast<uint32_t>(WPEFramework::Core::ERROR_GENERAL);
        }

        m_parent.m_interactionManager->tap();

        return static_cast<uint32_t>(WPEFramework::Core::ERROR_NONE);
    }

    WPEFramework::Exchange::IAVSController* ThunderInputManager::Controller()
    {
        return (&(*m_controller));
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
        CapabilitiesDelegateObserverInterface::State newState, 
		CapabilitiesDelegateObserverInterface::Error newError, 
		const std::vector< std::string > &addedOrUpdatedEndpointIds, 
		const std::vector< std::string > &deletedEndpointIds)
    {
        m_limitedInteraction = m_limitedInteraction || (newState == CapabilitiesDelegateObserverInterface::State::FATAL_ERROR);
    }

} // namespace Plugin
} // namespace WPEFramework

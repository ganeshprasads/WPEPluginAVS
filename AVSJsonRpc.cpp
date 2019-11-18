
#include "AVS.h"
#include "Module.h"

#include "ThunderInputManager.h"

/*
    // Copy the code below to AVS class definition
    // Note: The AVS class must inherit from PluginHost::JSONRPC

    private:
        void RegisterAll();
        void UnregisterAll();
        uint32_t endpoint_tap();
*/

namespace WPEFramework {

namespace Plugin {

    using namespace alexaClientSDK::sampleApp;
    // Registration
    //

    void AVS::RegisterAll()
    {
        Register<void, void>(_T("tap"), &AVS::endpoint_tap, this);
    }

    void AVS::UnregisterAll()
    {
        Unregister(_T("tap"));
    }

    // API implementation
    //

    // Method: tap - Start tap to talk
    // Return codes:
    //  - ERROR_NONE: Success
    //  - ERROR_UNAVAILABLE: AVS is unavailable
    //  - ERROR_INPROGRESS: There is already pending request
    //  - ERROR_GENERAL: Command failed to execute or does not exists
    uint32_t AVS::endpoint_tap()
    {
        uint32_t result = Core::ERROR_NONE;
        const InputCommand command = InputCommand::TAP;

        if (!_AVSDeviceClient) {
            result = Core::ERROR_UNAVAILABLE;
        } else {
            SampleAppReturnCode rc = _AVSDeviceClient->exec(command);
            switch (rc) {
            case SampleAppReturnCode::OK:
                break;
            case SampleAppReturnCode::BUSY:
                result = Core::ERROR_INPROGRESS;
                break;
            case SampleAppReturnCode::ERROR:
                result = Core::ERROR_GENERAL;
                break;
            default:
                result = Core::ERROR_GENERAL;
                break;
            }
        }

        return result;
    }

} // namespace Plugin
}

#pragma once

#include <aidl/com/myoem/hwbutton/BnHwButtonService.h>
#include <aidl/vendor/myoem/hardware/hwbutton/IHwButtonHal.h>

#include <mutex>
#include <vector>

namespace aidl::com::myoem::hwbutton {

// Thin orchestration layer: readValue()/triggerClick() pass straight
// through to the HAL; onHalClicked() fans out to registered framework
// callbacks. Never touches /dev/hwbutton directly (see HWBUTTON_TDD.md #4).
class HwButtonService : public BnHwButtonService {
public:
    explicit HwButtonService(
            std::shared_ptr<::aidl::vendor::myoem::hardware::hwbutton::IHwButtonHal> hal);

    ndk::ScopedAStatus readValue(int32_t* _aidl_return) override;
    ndk::ScopedAStatus triggerClick() override;
    ndk::ScopedAStatus registerCallback(
            const std::shared_ptr<IHwButtonServiceCallback>& callback) override;

    // Invoked by the HalCallbackShim (main.cpp) when the HAL reports a click.
    void onHalClicked();

private:
    std::shared_ptr<::aidl::vendor::myoem::hardware::hwbutton::IHwButtonHal> mHal;
    std::mutex mCallbacksMutex;
    std::vector<std::shared_ptr<IHwButtonServiceCallback>> mCallbacks;
};

}  // namespace aidl::com::myoem::hwbutton

#define LOG_TAG "hwbuttond"

#include <aidl/vendor/myoem/hardware/hwbutton/BnHwButtonHalCallback.h>
#include <android-base/logging.h>
#include <android/binder_manager.h>
#include <android/binder_process.h>

#include "HwButtonService.h"

using ::aidl::com::myoem::hwbutton::HwButtonService;
using ::aidl::vendor::myoem::hardware::hwbutton::BnHwButtonHalCallback;
using ::aidl::vendor::myoem::hardware::hwbutton::IHwButtonHal;

namespace {

// Bridges IHwButtonHalCallback (from the HAL) to HwButtonService::onHalClicked().
// Kept separate from HwButtonService itself to avoid diamond ambiguity between
// two unrelated Bn* base classes (see HWBUTTON_TDD.md #4.3).
class HalCallbackShim : public BnHwButtonHalCallback {
public:
    explicit HalCallbackShim(std::shared_ptr<HwButtonService> service)
        : mService(std::move(service)) {}

    ndk::ScopedAStatus onClicked() override {
        mService->onHalClicked();
        return ndk::ScopedAStatus::ok();
    }

private:
    std::shared_ptr<HwButtonService> mService;
};

}  // namespace

int main() {
    LOG(INFO) << "hwbuttond starting";

    const std::string halInstance =
            std::string() + IHwButtonHal::descriptor + "/default";
    ndk::SpAIBinder halBinder(AServiceManager_waitForService(halInstance.c_str()));
    CHECK(halBinder.get() != nullptr) << "failed to connect to " << halInstance;

    auto hal = IHwButtonHal::fromBinder(halBinder);
    CHECK(hal != nullptr) << "failed to cast binder to IHwButtonHal";

    auto service = ndk::SharedRefBase::make<HwButtonService>(hal);
    auto shim = ndk::SharedRefBase::make<HalCallbackShim>(service);
    ndk::ScopedAStatus status = hal->registerCallback(shim);
    CHECK(status.isOk()) << "failed to register HAL callback: " << status.getDescription();

    ABinderProcess_setThreadPoolMaxThreadCount(1);

    const std::string serviceInstance =
            std::string() + HwButtonService::descriptor + "/default";
    binder_status_t bstatus = AServiceManager_addService(
            service->asBinder().get(), serviceInstance.c_str());
    CHECK_EQ(bstatus, STATUS_OK) << "failed to register " << serviceInstance;

    LOG(INFO) << "hwbuttond registered as " << serviceInstance;

    ABinderProcess_startThreadPool();
    ABinderProcess_joinThreadPool();
    return 0;  // unreachable
}

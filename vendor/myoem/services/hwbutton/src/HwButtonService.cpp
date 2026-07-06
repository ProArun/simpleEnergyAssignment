#define LOG_TAG "HwButtonService"

#include "HwButtonService.h"

#include <android-base/logging.h>
#include <cerrno>

namespace aidl::com::myoem::hwbutton {

using ::aidl::vendor::myoem::hardware::hwbutton::IHwButtonHal;

HwButtonService::HwButtonService(std::shared_ptr<IHwButtonHal> hal) : mHal(std::move(hal)) {}

ndk::ScopedAStatus HwButtonService::readValue(int32_t* _aidl_return) {
    if (mHal == nullptr) {
        return ndk::ScopedAStatus::fromServiceSpecificError(EIO);
    }
    return mHal->getValue(_aidl_return);
}

ndk::ScopedAStatus HwButtonService::triggerClick() {
    if (mHal == nullptr) {
        return ndk::ScopedAStatus::fromServiceSpecificError(EIO);
    }
    return mHal->trigger();
}

ndk::ScopedAStatus HwButtonService::registerCallback(
        const std::shared_ptr<IHwButtonServiceCallback>& callback) {
    std::lock_guard<std::mutex> lock(mCallbacksMutex);
    mCallbacks.push_back(callback);
    return ndk::ScopedAStatus::ok();
}

void HwButtonService::onHalClicked() {
    LOG(INFO) << "onHalClicked: fanning out to " << mCallbacks.size() << " callback(s)";
    std::lock_guard<std::mutex> lock(mCallbacksMutex);
    for (auto& cb : mCallbacks) {
        cb->onClicked();
    }
}

}  // namespace aidl::com::myoem::hwbutton

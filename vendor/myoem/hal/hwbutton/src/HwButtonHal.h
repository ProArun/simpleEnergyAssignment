#pragma once

#include <aidl/vendor/myoem/hardware/hwbutton/BnHwButtonHal.h>

#include <atomic>
#include <mutex>
#include <thread>
#include <vector>

namespace aidl::vendor::myoem::hardware::hwbutton {

// Owns /dev/hwbutton. Sole process permitted to open the device node
// (see HWBUTTON_TDD.md #5.2) — the native service only ever talks to us
// over binder, never touches the device directly.
class HwButtonHal : public BnHwButtonHal {
public:
    // fd must already be open O_RDWR on /dev/hwbutton; HwButtonHal takes
    // ownership and closes it on destruction.
    explicit HwButtonHal(int fd);
    ~HwButtonHal() override;

    ndk::ScopedAStatus getValue(int32_t* _aidl_return) override;
    ndk::ScopedAStatus trigger() override;
    ndk::ScopedAStatus registerCallback(
            const std::shared_ptr<IHwButtonHalCallback>& callback) override;

private:
    void pollLoop();
    void notifyClicked();

    int mFd;
    std::thread mPollThread;
    std::atomic<bool> mRunning{true};
    std::mutex mCallbacksMutex;
    std::vector<std::shared_ptr<IHwButtonHalCallback>> mCallbacks;
};

}  // namespace aidl::vendor::myoem::hardware::hwbutton

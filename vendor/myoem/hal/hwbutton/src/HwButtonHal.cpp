#define LOG_TAG "HwButtonHal"

#include "HwButtonHal.h"

#include <android-base/logging.h>
#include <cerrno>
#include <poll.h>
#include <sys/eventfd.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include "hwbutton.h"

namespace aidl::vendor::myoem::hardware::hwbutton {

namespace {
// eventfd used purely to unblock poll() in the destructor so the poll
// thread can exit cleanly instead of blocking the process on shutdown.
int gStopEventFd = -1;
}  // namespace

HwButtonHal::HwButtonHal(int fd) : mFd(fd) {
    gStopEventFd = eventfd(0, EFD_NONBLOCK);
    mPollThread = std::thread(&HwButtonHal::pollLoop, this);
}

HwButtonHal::~HwButtonHal() {
    mRunning = false;
    if (gStopEventFd >= 0) {
        uint64_t one = 1;
        ssize_t ret = write(gStopEventFd, &one, sizeof(one));
        if (ret < 0) {
            PLOG(WARNING) << "failed to signal stop eventfd";
        }
    }
    if (mPollThread.joinable()) {
        mPollThread.join();
    }
    if (gStopEventFd >= 0) {
        close(gStopEventFd);
    }
    close(mFd);
}

ndk::ScopedAStatus HwButtonHal::getValue(int32_t* _aidl_return) {
    __u32 value = 0;
    if (ioctl(mFd, HWBUTTON_IOC_GET_VALUE, &value) < 0) {
        PLOG(ERROR) << "HWBUTTON_IOC_GET_VALUE failed";
        return ndk::ScopedAStatus::fromServiceSpecificError(errno);
    }
    *_aidl_return = static_cast<int32_t>(value);
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus HwButtonHal::trigger() {
    if (ioctl(mFd, HWBUTTON_IOC_TRIGGER) < 0) {
        PLOG(ERROR) << "HWBUTTON_IOC_TRIGGER failed";
        return ndk::ScopedAStatus::fromServiceSpecificError(errno);
    }
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus HwButtonHal::registerCallback(
        const std::shared_ptr<IHwButtonHalCallback>& callback) {
    std::lock_guard<std::mutex> lock(mCallbacksMutex);
    mCallbacks.push_back(callback);
    return ndk::ScopedAStatus::ok();
}

void HwButtonHal::notifyClicked() {
    std::lock_guard<std::mutex> lock(mCallbacksMutex);
    for (auto& cb : mCallbacks) {
        // oneway call — no cross-thread binder deadlock risk against the
        // binder thread pool servicing getValue/trigger/registerCallback.
        cb->onClicked();
    }
}

void HwButtonHal::pollLoop() {
    struct pollfd fds[2];
    fds[0].fd = mFd;
    fds[0].events = POLLIN;
    fds[1].fd = gStopEventFd;
    fds[1].events = POLLIN;

    while (mRunning) {
        fds[0].revents = 0;
        fds[1].revents = 0;

        int ret = poll(fds, 2, -1);
        if (ret < 0) {
            if (errno == EINTR) continue;
            PLOG(ERROR) << "poll() on /dev/hwbutton failed";
            break;
        }

        if (fds[1].revents & POLLIN) {
            break;  // stop requested
        }

        if (fds[0].revents & POLLIN) {
            LOG(INFO) << "hwbutton click detected via poll()";
            notifyClicked();
        }
    }
}

}  // namespace aidl::vendor::myoem::hardware::hwbutton

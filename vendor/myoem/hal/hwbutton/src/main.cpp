#define LOG_TAG "HwButtonHal"

#include <android-base/logging.h>
#include <android/binder_ibinder.h>
#include <android/binder_manager.h>
#include <android/binder_process.h>
#include <fcntl.h>

#include "HwButtonHal.h"

using aidl::vendor::myoem::hardware::hwbutton::HwButtonHal;

int main() {
    LOG(INFO) << "hwbutton-hal-default starting";

    int fd = open("/dev/hwbutton", O_RDWR);
    if (fd < 0) {
        PLOG(FATAL) << "failed to open /dev/hwbutton";
        return 1;
    }

    ABinderProcess_setThreadPoolMaxThreadCount(1);

    auto hal = ndk::SharedRefBase::make<HwButtonHal>(fd);

    const std::string instance =
            std::string() + HwButtonHal::descriptor + "/default";
    binder_status_t status = AServiceManager_addService(
            hal->asBinder().get(), instance.c_str());
    CHECK_EQ(status, STATUS_OK) << "failed to register " << instance;

    LOG(INFO) << "hwbutton-hal-default registered as " << instance;

    ABinderProcess_startThreadPool();
    ABinderProcess_joinThreadPool();
    return 0;  // unreachable
}

// CLI smoke-test client for com.myoem.hwbutton.IHwButtonService.
// Isolates native-stack bugs (kernel/HAL/hwbuttond) from framework/app bugs
// per HWBUTTON_FEATURE_SPEC.md Phase 3.
//
// Usage:
//   hwbutton_client read              - print current value
//   hwbutton_client trigger           - trigger a click
//   hwbutton_client watch             - register a callback and block,
//                                        printing "clicked" each time the
//                                        HAL->service->client chain fires

#include <aidl/com/myoem/hwbutton/BnHwButtonServiceCallback.h>
#include <aidl/com/myoem/hwbutton/IHwButtonService.h>
#include <android-base/logging.h>
#include <android/binder_manager.h>
#include <android/binder_process.h>

#include <cstdio>
#include <cstring>
#include <unistd.h>

using ::aidl::com::myoem::hwbutton::BnHwButtonServiceCallback;
using ::aidl::com::myoem::hwbutton::IHwButtonService;

namespace {

class WatchCallback : public BnHwButtonServiceCallback {
    ndk::ScopedAStatus onClicked() override {
        printf("clicked\n");
        fflush(stdout);
        return ndk::ScopedAStatus::ok();
    }
};

std::shared_ptr<IHwButtonService> connect() {
    const std::string instance = std::string() + IHwButtonService::descriptor + "/default";
    ndk::SpAIBinder binder(AServiceManager_waitForService(instance.c_str()));
    if (binder.get() == nullptr) {
        fprintf(stderr, "failed to connect to %s\n", instance.c_str());
        return nullptr;
    }
    return IHwButtonService::fromBinder(binder);
}

}  // namespace

int main(int argc, char** argv) {
    if (argc != 2) {
        fprintf(stderr, "usage: %s <read|trigger|watch>\n", argv[0]);
        return 1;
    }

    auto service = connect();
    if (service == nullptr) return 1;

    if (strcmp(argv[1], "read") == 0) {
        int32_t value = 0;
        ndk::ScopedAStatus status = service->readValue(&value);
        if (!status.isOk()) {
            fprintf(stderr, "readValue failed: %s\n", status.getDescription().c_str());
            return 1;
        }
        printf("%d\n", value);
        return 0;
    }

    if (strcmp(argv[1], "trigger") == 0) {
        ndk::ScopedAStatus status = service->triggerClick();
        if (!status.isOk()) {
            fprintf(stderr, "triggerClick failed: %s\n", status.getDescription().c_str());
            return 1;
        }
        printf("triggered\n");
        return 0;
    }

    if (strcmp(argv[1], "watch") == 0) {
        ABinderProcess_startThreadPool();
        auto cb = ndk::SharedRefBase::make<WatchCallback>();
        ndk::ScopedAStatus status = service->registerCallback(cb);
        if (!status.isOk()) {
            fprintf(stderr, "registerCallback failed: %s\n", status.getDescription().c_str());
            return 1;
        }
        printf("watching for clicks (Ctrl-C to stop)...\n");
        ABinderProcess_joinThreadPool();
        return 0;
    }

    fprintf(stderr, "unknown command: %s\n", argv[1]);
    return 1;
}

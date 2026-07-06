# HwButton — AOSP Hardware Button Simulation

Simulated hardware button wired through the full Android stack — kernel driver up to a
privileged app. Done for the Simple Energy AOSP Developer assignment, on a Raspberry Pi 5
running AOSP 15 (`android-15.0.0_r14`).

The "hardware" is fake (no real button, no GPIO) — the kernel driver just tracks a value
in memory. What's real is the full plumbing: a misc device with ioctl + poll, an AIDL
HAL, a C++ binder service, a Java system service, and an app on top, all talking to each
other the way an actual OEM stack would.

## Demo

[docs/demo.webm](docs/demo.webm)

<video src="docs/demo.webm" controls width="600"></video>

## What's here

```
vendor/myoem/
  kernel/hwbutton/       misc device driver — /dev/hwbutton, sysfs value/trigger
  hal/hwbutton/          AIDL HAL (hwbutton-hal-default), owns the device node
  services/hwbutton/     native AIDL service (hwbuttond), client of the HAL
  libs/hwbutton/         Java system service + app-facing Manager
  apps/HwButtonDemo/     Compose app — Read / Trigger buttons

frameworks/base/
  services/java/.../SystemServer.java     one call site to start the service
  services/core/Android.bp                 wires HwButtonManagerService into services.jar
```

The `frameworks/base` bit is the one part that isn't under `vendor/myoem`. Wanted a real
`SystemService` running inside `system_server`, not just a helper class the app
constructs itself — that needs `SystemServer` to actually start it, so it's a small,
deliberate exception to keeping everything vendor-side.

## The stack

```
App (HwButtonDemo)
  -> HwButtonManager                     ServiceManager.getService("hwbutton")
  -> HwButtonManagerService              real SystemService inside system_server
  -> hwbuttond                           native AIDL service, vendor process
  -> hwbutton-hal-default                AIDL HAL, owns /dev/hwbutton
  -> hwbutton.ko                         misc device, ioctl + poll
```

**Read** is a plain synchronous call down and back.

**Trigger** does something more interesting: it writes to the driver, which logs
`hwbutton: clicked` to dmesg and resets to 0, and *also* wakes up a `poll()` call sitting
in the HAL. That wakeup gets pushed back up the whole chain as an async callback
(`onClicked()`), independent of the trigger call's own return. The app's "Clicked" toast
is driven by that callback, not by the synchronous return — otherwise there'd be no real
proof the poll() path does anything.

## Building

```
source build/envsetup.sh
lunch myoem_rpi5-trunk_staging-userdebug
make bootimage systemimage vendorimage -j$(nproc)
```

Everything under `vendor/myoem` builds as normal Soong modules. The kernel driver
(`kernel/hwbutton/hwbutton.c`) is **not** part of that build — Soong doesn't build kernel
modules, so it needs a separate out-of-tree build against a matching kernel source:

```
cd vendor/myoem/kernel/hwbutton
make -C <kernel-source-dir> M=$(pwd) ARCH=arm64 CROSS_COMPILE=aarch64-linux-android- modules
```

I validated the driver logic by building it out-of-tree against a plain x86_64 Linux
kernel on the dev machine (zero warnings) — there's no RPi5-specific code in it, it's a
generic misc device. Getting it running on the actual board needs the kernel source that
matches the RPi5 image's exact build (`device/brcm/rpi5-kernel` in this tree only ships a
prebuilt `Image`, not source), which I didn't have on hand. That's the one gap between
"builds clean" and "dmesg proof from real hardware."

## Testing it

Once flashed:

```
adb shell ps -eZ | grep hwbutton
adb shell service list | grep -i hwbutton
adb shell hwbutton_client read
adb shell hwbutton_client trigger
adb shell dmesg | tail
adb logcat -d | grep "avc: denied"
```

`hwbutton_client` is a small CLI in `services/hwbutton/test/` — useful for checking the
kernel → HAL → native chain works before dragging the framework and app into it.

For SELinux: everything's meant to run under enforcing. `hwbutton_hal` is the only domain
allowed to touch the device node; `hwbuttond` only gets binder access to the HAL, nothing
below it.

## Notes / trade-offs

- HAL is a real separate process, not a passthrough library — heavier than this hardware
  strictly needs, but the assignment lists HAL and native service as two separate stack
  layers, which reads as wanting the actual binderized pattern.
- App looks up the Manager with `ServiceManager.getService()` rather than
  `Context.getSystemService(Class)`. The latter needs the Manager class on the boot
  classpath plus hidden-API bookkeeping — real infra, but more than this assignment
  needs. The service itself is still a genuine `SystemService` inside `system_server`.
- Kernel driver tracks a `click_seq` counter rather than just the button value for
  `poll()` — the value resets to 0 right after a trigger, so comparing raw value could
  miss a click on a race. Sequence number doesn't have that problem.

Build/run log with the actual errors hit along the way (there were a few — namespace
rules in Soong, AIDL versioning, a wrong sepolicy attribute, R8 needing static libs
instead of libs) is worth a separate writeup if useful; happy to walk through any of it.

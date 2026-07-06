KERNEL_DIR ?= /home/arun/aosp-rpi/kernel
ARCH ?= arm64
CROSS_COMPILE ?= aarch64-linux-android-

obj-m += hwbutton.o

.PHONY: all clean

all:
	$(MAKE) -C $(KERNEL_DIR) M=$(CURDIR) ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) modules

clean:
	$(MAKE) -C $(KERNEL_DIR) M=$(CURDIR) ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) clean

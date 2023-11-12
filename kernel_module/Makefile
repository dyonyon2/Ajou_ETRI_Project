obj-m += hello_module.o

#you can change cross_conpile for you system.
ARCH=arm
CROSS_COMPILE=arm-linux-gnueabihf-
V2X_SDK_PATH   :=/home/dyonyon/Desktop/ETRI/v2x-sdk

# check ther Kernel_directory
KERNEL_ARM_DIR=$(V2X_SDK_PATH)/usr/src/linux
KERNEL_ARM_INC=$(KERNEL_ARM_DIR)/include


.PHONY: all clean 

all:
	make ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) -C $(KERNEL_ARM_DIR) M=$(PWD) modules

clean:
	rm -f *~ .tmp_versions/*.mod  .*.cmd Module.markers Module.symvers *.o modules.order *.ko *.mod.c


export ARCH := arm

# arm-linux-gnueabihf-gcc (crosstool-NG linaro-1.13.1-4.9-2014.09 - Linaro GCC 4.9-2014.09) 4.9.2 20140904 (prerelease)
export TOOLCHAIN_DIR := ~hps/BR/HOST/usr/bin/

# arm-linux-gnueabihf-gcc (Ubuntu/Linaro 4.6.3-1ubuntu5) 4.6.3
#export TOOLCHAIN_DIR := /usr/bin/

export CROSS_COMPILE := $(TOOLCHAIN_DIR)arm-linux-gnueabihf-
export CC := $(CROSS_COMPILE)gcc
export LD := $(CROSS_COMPILE)ld

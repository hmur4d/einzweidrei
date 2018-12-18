# doesn't honor Visual Studio's platform & configuration settings
# it doesn't make sense to build the module for something else than the default ARM target.

# BASEDIR depends on the build step: 
# when M is defined, we're called from the kernel's Makefile, and M contains the module path
# otherwise, we're called directly from this Makefile so we use a relative path
BASEDIR := $(if $(M),$(M)/..,..)
include $(BASEDIR)/make-config/cross-compile.mk
include $(BASEDIR)/make-config/kernel.mk

# we need to specify all files here, for multifile module
# for some reason, using wildcards and filter-out ".mod.c" doesn't work in this context
obj-m := modcameleon.o
modcameleon-objs := dynamic_device.o interrupt_queue.o gpio_irq.o dev_interrupts.o dev_rxdata.o fpga_bridges.o module.o

CFLAGS_MODULE += -fno-pic -Wno-declaration-after-statement

# -std=gnu11 doesn't work with buildroot's toolchain 
# it does with a more recent gcc such as the toolchain included in "WSL/Windows"'s Ubuntu.
#CFLAGS_MODULE += -std=gnu11

all:
	$(MAKE) -C $(KERNEL_DIR) M=`pwd`
	
clean:
	rm -f *.o *~ core .depend .*.cmd *.ko *.mod.c
	rm -f Module.markers Module.symvers modules.order
	rm -rf .tmp_versions Modules.symvers

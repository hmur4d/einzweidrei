# besoin d'un chemin absolu car appelé depuis le Makefile du kernel
include ~/.vstudio-vfiack/make-config/cross-compile.mk
include ~/.vstudio-vfiack/make-config/kernel.mk

# ne compile pas avec le toolchain buildroot & l'option -std=gnu11
# fonctionne sans l'option
# fonction avec l'option sur un gcc plus récent (ex: toolchain packagé sur ubuntu "WSL/Windows")
#1>module\modsample.c(36,1): error : initializer element is not constant

# needs to specify all files here, for multifile module
# for some reason, using wildcards and filter-out ".mod.c" doesn't work in this context
obj-m := modcameleon.o
modcameleon-objs := dynamic_device.o blocking_queue.o gpio_irq.o dev_interrupts.o hps2fpga.o module.o

CFLAGS_MODULE += -fno-pic -Wno-declaration-after-statement
#CFLAGS_MODULE += -std=gnu11

all:
	$(MAKE) -C $(KERNEL_DIR) M=`pwd`
	
clean:
	rm -f *.o *~ core .depend .*.cmd *.ko *.mod.c
	rm -f Module.markers Module.symvers modules.order
	rm -rf .tmp_versions Modules.symvers

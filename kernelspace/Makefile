include ~/projects/make-config/cross-compile.mk

# ne compile pas avec le toolchain buildroot & l'option -std=gnu11
# fonctionne sans l'option
# fonction avec l'option sur un gcc plus récent (ex: toolchain packagé sur ubuntu "WSL/Windows")
#1>module\modsample.c(36,1): error : initializer element is not constant

MODULE_NAME := modcameleon
KDIR := ~/BR/KERNEL

CFLAGS_MODULE += -fno-pic -Wno-declaration-after-statement
#CFLAGS_MODULE += -std=gnu11

obj-m := $(MODULE_NAME).o

all:
	$(MAKE) -C $(KDIR) M=`pwd`
	
clean:
	rm -f *.o *~ core .depend .*.cmd *.ko *.mod.c
	rm -f Module.markers Module.symvers modules.order
	rm -rf .tmp_versions Modules.symvers

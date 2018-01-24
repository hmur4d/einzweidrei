include make-config/cross-compile.mk make-config/ssh.mk

all: user kernel

clean: userclean kernelclean

user:
	$(MAKE) -C userspace

kernel:
	$(MAKE) -C kernelspace

	
user-clean:
	$(MAKE) -C userspace clean
	
kernel-clean:
	$(MAKE) -C userspace clean


#-------------------------
# inutilisé pour l'instant, copie des tests faits avant la carte reflex
#-------------------------

prepare: remoteclean copy
	$(SSH) $(SSH_HOST) "insmod $(MODULE).ko"

remoteclean:
	$(SSH) $(SSH_HOST) "killall -9 $(TARGET) ; rmmod $(MODULE).ko ; true"
	
copy: $(TARGET) module
	$(SCP) module/$(MODULE).ko $(SSH_HOST):/root/
	$(SCP) $(TARGET) $(SSH_HOST):/root/
	$(SSH) $(SSH_HOST) "chmod a+x $(TARGET)"
	
run: prepare
	$(SSH) $(SSH_HOST) "./$(TARGET)"
	
debug: prepare
	$(SSH) $(SSH_HOST) "gdbserver :9999 ./$(TARGET)" &
	cat ~/buildroot/output/staging/usr/share/buildroot/gdbinit > /tmp/gdbinit
	echo "target remote $(HOST):9999" >> /tmp/gdbinit
	sleep 1
	~/buildroot/output/host/usr/bin/arm-linux-gdb -x /tmp/gdbinit $(TARGET)
	

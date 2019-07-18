include make-config/cross-compile.mk make-config/ssh.mk

all: clean compile zip

#--

clean: user-clean kernel-clean
	rm -rf archive
	rm -f *.zip

compile: user kernel

zip:
	mkdir archive
	cp userspace/cameleon archive
	cp userspace/config/* archive
	cp userspace/scripts/* archive
	cp kernelspace/modcameleon.ko archive
	zip -j archive.zip archive/*

#--

user:
	$(MAKE) -C userspace

kernel:
	$(MAKE) -C kernelspace


user-clean:
	$(MAKE) -C userspace clean

kernel-clean:
	$(MAKE) -C userspace clean


=== README.txt: cameleon userspace ==



== Using environnment variables to change userspace behaviour

= setting log level

By default, the userspace program will show logs of level >= INFO.
It is possible to change this by setting the "LOG_LEVEL" environment variable. 
Possible values are: ALL, DEBUG, INFO, WARNING, ERROR, ALL

= setting /dev/mem file

By default, the userspace program use /dev/mem to map the FPGA memory
It is possible to change this by setting the "DEV_MEM" environment variable to another (existing) file.
This is useful to run on a computer without FPGA, for debugging purposes.



== Debugging userspace on an x86 linux system

= accessing shared memory

First create a big enough fake memory file:
# dd if=/dev/zero of=/tmp/fakemem bs=1G count=4
# truncate -s 4G /tmp/fakemem

Then set the DEV_MEM environment variable:
# export DEV_MEM=/tmp/fakemem

It is possible to inspect the file content with "od":
options: 
	-j to skips to an address (here, the lightweight bus)
	-Ax to show the address in hex
	-xd to show the values both in hex and in decimal
# od -Ax -j 0xFF200000 -xd /tmp/fakemem

= sending interruptions

Before the first start, create the /dev/interrupts fifo
# mkfifo /dev/interrupts && chmod a+rw /dev/interrupts

Then before starting the userspace program, have a process using the fifo so it stays open:
# cat > /dev/interrupts &

Finally, to send interrupts, we can use echo to send bytes:
options: 
  -n to skip the newline
  -e to accept hex bytes

# echo -n -e \\x02 > /dev/interrupts

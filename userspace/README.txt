=== 

== Debugging userspace on an x86 linux system

= sending interruptions

Before the first start, create the /dev/interrupts fifo
# mkfifo /dev/interrupts && chmod a+rw /dev/interrupts

Then before starting the userspace program, have a process using the fifo so it stays open:
# cat > /dev/interrupts &

Finally, to send interrupts, we can use echo to send bytes:
options: 
  -n to skip the newline
  -e to accept hex bytes

# echo -n -e \\x7B > /dev/interrupts

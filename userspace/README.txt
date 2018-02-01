=== 

== Using environnment variables to change userspace behaviour

= setting log level

By default, the userspace program will show logs of level >= INFO.
It is possible to change this by setting the "LOGLEVEL" environment variable. 
Possible values are: ALL, DEBUG, INFO, WARNING, ERROR, ALL

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

# echo -n -e \\x02 > /dev/interrupts

#!/bin/sh

HOST=192.168.1.155
SSH_HOST=root@$HOST
SSH="sshpass -f make-config/ssh.password ssh -o UserKnownHostsFile=/dev/null -o StrictHostKeyChecking=no"
SCP="sshpass -f make-config/ssh.password scp -o UserKnownHostsFile=/dev/null -o StrictHostKeyChecking=no"

$SSH $SSH_HOST "killall -9 cameleon ; rmmod modcameleon.ko ; true"
$SCP userspace/bin/ARM/Debug/cameleon $SSH_HOST:/root
$SCP kernelspace/bin/ARM/Debug/modcameleon.ko $SSH_HOST:/root
$SSH $SSH_HOST "chmod a+x /root/cameleon"
$SSH $SSH_HOST "/root/cameleon"

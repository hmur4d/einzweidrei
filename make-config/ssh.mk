HOST=192.168.1.155
SSH_HOST=root@$(HOST)
SSH=sshpass -f make-config/ssh.password ssh -o UserKnownHostsFile=/dev/null -o StrictHostKeyChecking=no
SCP=sshpass -f make-config/ssh.password scp -o UserKnownHostsFile=/dev/null -o StrictHostKeyChecking=no

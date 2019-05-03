#!/bin/sh

source /opt/RS2D/cameleon.conf

rmmod modcameleon.ko
insmod /opt/RS2D/modcameleon.ko
/opt/RS2D/cameleon

Instruction can be found here:
http://derekmolloy.ie/writing-a-linux-kernel-module-part-1-introduction/

The code is tested on RPI 3 device with Raspbian OS.

# Terminal Commands

## Build module binary
`make`

## Install module binary
`sudo insmod MODULE_NAME.ko`

## List installed modules
`lsmod`

## Check module info
`modinfo MODULE_NAME.ko`

## Uninstall module
`sudo rmmod MODULE_NAME.ko`

## View kernal log
`tail -f /var/log/kern.log`

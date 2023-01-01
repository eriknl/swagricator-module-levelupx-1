#!/bin/sh
echo "Setting up 'Module LevelUpX-1'"

passwd -l root 2> /dev/null
echo "Root login disabled"
echo "ttyAMA0::respawn:/sbin/getty -L 0 ttyAMA0 vt100" > /etc/inittab
kill -1 1
echo "Console locked"

if [ -z `grep levelupx /etc/passwd` ]; then
    echo "Adding user 'levelupx'"
    adduser -h /mnt/module -s /mnt/module/levelupx-1 levelupx -D
    echo "levelupx:levelupx" | chpasswd 2> /dev/null
fi
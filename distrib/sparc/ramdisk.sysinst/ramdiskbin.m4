#	$NetBSD: ramdiskbin.m4,v 1.4 2000/08/22 14:11:46 abs Exp $
#
# ramdiskbin.conf - unified binary for the install ramdisk
#

srcdirs bin sbin usr.bin/less usr.bin usr.sbin gnu/usr.bin sys/arch/MACHINE/stand

progs cat chmod chown chroot cp dd df disklabel dmesg ed
progs fsck fsck_ffs ftp gzip ifconfig init installboot
progs less ln ls mkdir mknod mount mount_cd9660
progs mount_ffs mount_msdos mount_nfs mt mv newfs pax
progs ping pwd reboot restore rm route sed sh shutdown
progs slattach swapctl sync sysinst test stty tip umount

ifelse(MACHINE,i386,progs bad144 fdisk mbrlabel mount_ext2fs)
ifelse(MACHINE,sparc,progs getopt sysctl) # Used by binstall

special sysinst srcdir distrib/utils/sysinst/arch/MACHINE
special init srcdir distrib/utils/init_s

special dd srcdir distrib/utils/x_dd
special dmesg srcdir distrib/utils/x_dmesg
special ed srcdir distrib/utils/x_ed
special ftp srcdir distrib/utils/x_ftp
special ifconfig srcdir distrib/utils/x_ifconfig
special ping srcdir distrib/utils/x_ping
special route srcdir distrib/utils/x_route
special sh srcdir distrib/utils/x_sh

# "special" gzip is actually larger assuming nothing else uses -lz..
#special gzip srcdir distrib/utils/x_gzip
#x_dhclient, x_netstat, x_ping6 not current used

ln pax tar
ln chown chgrp
ln gzip gzcat gunzip
ln less more
ln sh -sh		# init invokes the shell this way
ln test [
ln mount_cd9660 cd9660
ln mount_ffs ffs
ln mount_msdos msdos
ln mount_nfs nfs
ln reboot halt
ln restore rrestore

# libhack.o is built by Makefile & included Makefile.inc
libs libhack.o -ledit -lutil -lcurses -ltermcap -lrmt -lbz2 -lcrypt -ll -lm

#	$NetBSD: ramdiskbin.m4,v 1.1 1999/04/30 05:09:44 abs Exp $
#
# ramdiskbin.conf - unified binary for the install ramdisk
#

srcdirs bin sbin usr.bin/less usr.bin usr.sbin gnu/usr.bin sys/arch/MACHINE/stand

progs cat chmod chown chroot cp dd df disklabel ed
progs fsck fsck_ffs ftp gzip ifconfig init installboot less
progs ln ls mkdir mknod
progs mount mount_cd9660 mount_ext2fs mount_ffs mount_msdos
progs mount_nfs mount_kernfs mt mv newfs ping pwd reboot restore rm
progs route sed sh shutdown slattach stty swapctl sync test
progs tip umount update
progs sysinst pax
ifelse(MACHINE,i386,progs bad144 fdisk mbrlabel)
ifelse(MACHINE,sparc,progs sysctl getopt)

special sysinst srcdir distrib/utils/sysinst/arch/MACHINE
special init srcdir distrib/utils/init_s

special dd srcdir distrib/utils/x_dd
special ftp srcdir distrib/utils/x_ftp
special ifconfig srcdir distrib/utils/x_ifconfig
special route srcdir distrib/utils/x_route
special sh srcdir distrib/utils/x_sh

# "special" gzip is actually larger assuming nothing else uses -lz..
#special gzip srcdir distrib/utils/x_gzip

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
ln mount_kernfs kernfs
ln reboot halt
ln restore rrestore

# libhack.o is built by Makefile & included Makefile.inc
libs libhack.o -ledit -lutil -lcurses -ltermcap -lrmt -lcrypt -ll -lm

#	$NetBSD: ramdiskbin.m4,v 1.1 2001/01/07 09:30:20 mrg Exp $
#
# ramdiskbin.conf - unified binary for the install ramdisk
#

srcdirs bin sbin libexec usr.bin/less usr.bin usr.sbin gnu/usr.bin sys/arch/MACHINE/stand

progs cat chmod chown chroot cp dd df disklabel dhclient ed
progs fsck fsck_ffs ftp gzip ifconfig init installboot less
progs lfs_cleanerd ln ls mkdir mknod
progs mount mount_cd9660 mount_ext2fs mount_ffs mount_lfs mount_msdos
progs mount_nfs mount_kernfs mt mv newfs ping newfs_lfs pwd reboot restore rm
progs route sed sh shutdown slattach stty swapctl sync test
progs tip umount
progs sysinst pax
ifelse(MACHINE,i386,progs bad144 fdisk mbrlabel)
ifelse(MACHINE,sparc,progs sysctl getopt)
ifelse(MACHINE,sparc64,progs sysctl getopt pppd chat rcp rcmd)

special sysinst srcdir distrib/utils/sysinst/arch/MACHINE
special init srcdir distrib/utils/init_s

special dd srcdir distrib/utils/x_dd
special ftp srcdir distrib/utils/x_ftp
special ifconfig srcdir distrib/utils/x_ifconfig
special route srcdir distrib/utils/x_route
ifelse(MACHINE,sparc64,,special sh srcdir distrib/utils/x_sh)
special ping srcdir distrib/utils/x_ping

ifelse(MACHINE,sparc64,special pppd srcdir usr.sbin/pppd/pppd)
ifelse(MACHINE,sparc64,special chat srcdir usr.sbin/pppd/chat)
ifelse(MACHINE,sparc64,special installboot srcdir sys/arch/sparc/stand/installboot)

# "special" gzip is actually larger assuming nothing else uses -lz..
#special gzip srcdir distrib/utils/x_gzip

special dhclient srcdir distrib/utils/x_dhclient

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
ln newfs mount_mfs

# libhack.o is built by Makefile & included Makefile.inc
libs libhack.o -ledit -lutil -lcurses -ltermcap -lrmt -lbz2 -lcrypt -ll -lm ifelse(MACHINE,sparc64,-lpcap)

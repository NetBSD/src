#	$NetBSD: list.m4,v 1.5 2000/03/15 12:19:16 soren Exp $

# copy the crunched binary, link to it, and kill it
COPY	${OBJDIR}/ramdiskbin		ramdiskbin
LINK	ramdiskbin			sysinst
LINK	ramdiskbin			bin/cat
LINK	ramdiskbin			bin/chmod
LINK	ramdiskbin			bin/cp
LINK	ramdiskbin			bin/dd
LINK	ramdiskbin			bin/df
LINK	ramdiskbin			bin/ed
LINK	ramdiskbin			bin/ln
LINK	ramdiskbin			bin/ls
LINK	ramdiskbin			bin/mkdir
LINK	ramdiskbin			bin/mt
LINK	ramdiskbin			bin/mv
LINK	ramdiskbin			bin/pax
LINK	ramdiskbin			bin/pwd
LINK	ramdiskbin			bin/rm
LINK	ramdiskbin			bin/sh
LINK	ramdiskbin			bin/stty
LINK	ramdiskbin			bin/sync
LINK	ramdiskbin			bin/test
LINK	ramdiskbin			bin/[
LINK	ramdiskbin			sbin/cd9660
LINK	ramdiskbin			sbin/disklabel
LINK	ramdiskbin			sbin/ffs
LINK	ramdiskbin			sbin/fsck
LINK	ramdiskbin			sbin/fsck_ffs
LINK	ramdiskbin			sbin/halt
LINK	ramdiskbin			sbin/ifconfig
LINK	ramdiskbin			sbin/init
LINK	ramdiskbin			sbin/kernfs
LINK	ramdiskbin			sbin/mknod
LINK	ramdiskbin			sbin/mount
LINK	ramdiskbin			sbin/mount_cd9660
LINK	ramdiskbin			sbin/mount_ext2fs
LINK	ramdiskbin			sbin/mount_ffs
LINK	ramdiskbin			sbin/mount_kernfs
LINK	ramdiskbin			sbin/mount_msdos
LINK	ramdiskbin			sbin/mount_nfs
LINK	ramdiskbin			sbin/msdos
LINK	ramdiskbin			sbin/newfs
LINK	ramdiskbin			sbin/nfs
LINK	ramdiskbin			sbin/ping
LINK	ramdiskbin			sbin/reboot
LINK	ramdiskbin			sbin/restore
LINK	ramdiskbin			sbin/route
LINK	ramdiskbin			sbin/rrestore
LINK	ramdiskbin			sbin/shutdown
LINK	ramdiskbin			sbin/slattach
LINK	ramdiskbin			sbin/swapctl
LINK	ramdiskbin			sbin/umount
ifelse(MACHINE,i386,	LINK	ramdiskbin	sbin/fdisk)
ifelse(MACHINE,i386,	LINK	ramdiskbin	sbin/mbrlabel)
SYMLINK	/bin/cat		usr/bin/chgrp
SYMLINK	/bin/cat		usr/bin/ftp
SYMLINK	/bin/cat		usr/bin/gunzip
SYMLINK	/bin/cat		usr/bin/gzcat
SYMLINK	/bin/cat		usr/bin/gzip
SYMLINK	/bin/cat		usr/bin/less
SYMLINK	/bin/cat		usr/bin/more
SYMLINK	/bin/cat		usr/bin/sed
SYMLINK	/bin/cat		usr/bin/tar
SYMLINK	/bin/cat		usr/bin/tip
SYMLINK	/bin/cat		usr/mdec/installboot
SYMLINK	/bin/cat		usr/sbin/chown
SYMLINK	/bin/cat		usr/sbin/chroot
ifelse(MACHINE,i386,	SYMLINK	/bin/cat	usr/sbin/bad144)
ifelse(MACHINE,sparc,	SYMLINK	/bin/cat	usr/bin/getopt)
ifelse(MACHINE,sparc,	SYMLINK	/bin/cat	sbin/sysctl)
SPECIAL	/bin/rm ramdiskbin

# various files that we need in /etc for the install
COPY	SRCROOT/etc/group		etc/group
COPY	SRCROOT/etc/master.passwd	etc/master.passwd
COPY	SRCROOT/etc/protocols	etc/protocols
COPY	SRCROOT/etc/services	etc/services

SPECIAL	pwd_mkdb -p -d ./ etc/master.passwd
SPECIAL /bin/rm etc/spwd.db
SPECIAL /bin/rm etc/pwd.db

# copy the MAKEDEV script and make some devices
COPY	SRCROOT/etc/etc.MACHINE/MAKEDEV	dev/MAKEDEV
SPECIAL	cd dev; sh MAKEDEV ramdisk
SPECIAL	/bin/rm dev/MAKEDEV

# we need the boot block in /usr/mdec + the arch specific extras
ifelse(MACHINE,sparc, COPY ${DESTDIR}/usr/mdec/boot usr/mdec/boot)
ifelse(MACHINE,sparc, COPY ${DESTDIR}/usr/mdec/bootxx usr/mdec/bootxx)
ifelse(MACHINE,sparc, COPY ${DESTDIR}/usr/mdec/binstall usr/mdec/binstall)
ifelse(MACHINE,i386,  COPY ${DESTDIR}/usr/mdec/biosboot.sym usr/mdec/biosboot.sym)
ifelse(MACHINE,i386,  COPY ${DESTDIR}/usr/mdec/mbr         usr/mdec/mbr)
ifelse(MACHINE,i386,  COPY ${DESTDIR}/usr/mdec/mbr_bootsel	usr/mdec/mbr_bootsel)

# and the common installation tools
COPY	termcap.mini		usr/share/misc/termcap

# the disktab explanation file
COPY	disktab.preinstall		etc/disktab.preinstall

#the lists of obsolete files used by sysinst
COPY dist/base_obsolete dist/base_obsolete
COPY dist/comp_obsolete dist/comp_obsolete
COPY dist/games_obsolete dist/games_obsolete
COPY dist/man_obsolete dist/man_obsolete
COPY dist/misc_obsolete dist/misc_obsolete
COPY dist/secr_obsolete dist/secr_obsolete
COPY dist/xbase_obsolete dist/xbase_obsolete
COPY dist/xserver_obsolete dist/xserver_obsolete

# and the installation tools
COPY	${OBJDIR}/dot.profile			.profile

#the lists of obsolete files used by sysinst  
SPECIAL sh ${CURDIR}/../../sets/makeobsolete -b -s ${CURDIR}/../../sets -t ./dist

#	$NetBSD: fstab.rd,v 1.1.8.1 2005/04/04 17:30:53 tron Exp $
#
# Sample fstab file for hp300 rd(4) disks.
#
/dev/rd0a	/	ffs	rw		1 1
/dev/rd0e	/usr	ffs	rw		1 2
/dev/rd0f	/var	ffs	rw		1 2
#
# Possibly include data from the following files here:
# fstab.cdrom
# fstab.pseudo
# fstab.ramdisk

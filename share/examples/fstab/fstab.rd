#	$NetBSD: fstab.rd,v 1.2 2005/04/03 14:12:14 hubertf Exp $
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

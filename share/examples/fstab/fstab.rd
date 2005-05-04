#	$NetBSD: fstab.rd,v 1.3 2005/05/04 18:26:14 hubertf Exp $
#
# Sample fstab file for hp300 rd(4) disks.
#
/dev/rd0a	/	ffs	rw		1 1
/dev/rd0e	/usr	ffs	rw,nodev	1 2
/dev/rd0f	/var	ffs	rw,nodev,nosuid	1 2
#
# Possibly include data from the following files here:
# fstab.cdrom
# fstab.pseudo
# fstab.ramdisk

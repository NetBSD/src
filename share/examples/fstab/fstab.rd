#	$NetBSD: fstab.rd,v 1.1 2002/05/04 19:44:35 jdolecek Exp $
#
# Sample fstab file for hp300 rd(4) disks.
#
/dev/rd0a	/	ffs	rw		1 1
/dev/rd0e	/usr	ffs	rw		1 2
/dev/rd0f	/var	ffs	rw		1 2
/kern		/kern	kernfs	ro		0 0
/proc		/proc	procfs	ro		0 0
fdesc		/dev	fdesc   ro,-o=union	0 0

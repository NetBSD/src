#	$NetBSD: Makefile.inc,v 1.1 2000/06/12 12:41:40 shin Exp $

#EXTRA_KERNELS+= 	TX3912 TX3922
BUILD_KERNELS+= 	RAMDISK

snap_md_post:
.ifndef UPDATE
	cd ${.CURDIR}/../distrib && ${MAKE} cleandir
.endif
	cd ${.CURDIR}/../distrib && ${MAKE} depend && ${MAKE}
	cd ${.CURDIR}/../distrib/hpcmips && ${MAKE} release
	cd ${.CURDIR}/../distrib/notes && ${MAKE} release

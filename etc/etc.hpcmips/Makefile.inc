#	$NetBSD: Makefile.inc,v 1.2 2000/06/17 10:59:27 shin Exp $

EXTRA_KERNELS+= 	TX3912 TX3922
BUILD_KERNELS+= 	RAMDISK

snap_md_post:
.ifndef UPDATE
	cd ${.CURDIR}/../distrib && ${MAKE} cleandir
.endif
	cd ${.CURDIR}/../distrib && ${MAKE} depend && ${MAKE}
	cd ${.CURDIR}/../distrib/hpcmips && ${MAKE} release
	cd ${.CURDIR}/../distrib/notes && ${MAKE} release

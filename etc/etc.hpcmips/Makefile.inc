#	$NetBSD: Makefile.inc,v 1.3 2001/04/22 02:03:05 shin Exp $

EXTRA_KERNELS+= 	TX3912 TX3922
BUILD_KERNELS+= 	RAMDISK INSTALL_TX3912 INSTALL_TX3922

snap_md_post:
.ifndef UPDATE
	cd ${.CURDIR}/../distrib && ${MAKE} cleandir
.endif
	cd ${.CURDIR}/../distrib && ${MAKE} depend && ${MAKE}
	cd ${.CURDIR}/../distrib/hpcmips && ${MAKE} release
	cd ${.CURDIR}/../distrib/notes && ${MAKE} release

#	$NetBSD: Makefile.inc,v 1.1 2001/10/15 16:22:53 uch Exp $

BUILD_KERNELS+= 	RAMDISK

snap_md_post:
.ifndef UPDATE
	cd ${.CURDIR}/../distrib && ${MAKE} cleandir
.endif
	cd ${.CURDIR}/../distrib && ${MAKE} depend && ${MAKE}
	cd ${.CURDIR}/../distrib/playstation2 && ${MAKE} release
	cd ${.CURDIR}/../distrib/notes && ${MAKE} release

#	$NetBSD: Makefile.inc,v 1.2 2001/11/29 22:45:58 thorpej Exp $

KERNEL_SETS=		GENERIC

BUILD_KERNELS=	 	RAMDISK

snap_md_post:
.ifndef UPDATE
	cd ${.CURDIR}/../distrib && ${MAKE} cleandir
.endif
	cd ${.CURDIR}/../distrib && ${MAKE} depend && ${MAKE}
	cd ${.CURDIR}/../distrib/playstation2 && ${MAKE} release
	cd ${.CURDIR}/../distrib/notes && ${MAKE} release

#	$NetBSD: Makefile.inc,v 1.1 2002/04/14 14:20:05 tsutsui Exp $

KERNEL_SETS=		GENERIC

BUILD_KERNELS=		RAMDISK

snap_md_post:
.ifndef UPDATE
	cd ${.CURDIR}/../distrib && ${MAKE} cleandir
.endif
	cd ${.CURDIR}/../distrib && ${MAKE} depend && ${MAKE}
	cd ${.CURDIR}/../distrib/${MACHINE} && ${MAKE} release
	cd ${.CURDIR}/../distrib/notes && ${MAKE} release

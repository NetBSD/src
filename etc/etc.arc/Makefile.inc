#	$NetBSD: Makefile.inc,v 1.2 2002/04/15 14:41:39 tsutsui Exp $

KERNEL_SETS=		GENERIC
KERNEL_SUFFIXES=	ecoff

BUILD_KERNELS=		RAMDISK

snap_md_post:
.ifndef UPDATE
	cd ${.CURDIR}/../distrib && ${MAKE} cleandir
.endif
	cd ${.CURDIR}/../distrib && ${MAKE} depend && ${MAKE}
	cd ${.CURDIR}/../distrib/${MACHINE} && ${MAKE} release
	cd ${.CURDIR}/../distrib/notes && ${MAKE} release

#	$NetBSD: Makefile.inc,v 1.1 2002/02/03 11:23:45 tsutsui Exp $

KERNEL_SETS=		GENERIC

BUILD_KERNELS=		INSTALL

snap_md_post:
.ifndef UPDATE
	cd ${.CURDIR}/../distrib && ${MAKE} cleandir
.endif
	cd ${.CURDIR}/../distrib && ${MAKE} depend && ${MAKE}
	cd ${.CURDIR}/../distrib/newsmips && ${MAKE} release
	cd ${.CURDIR}/../distrib/notes && ${MAKE} release

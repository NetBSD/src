#	$NetBSD: Makefile.inc,v 1.1 2002/09/21 01:41:32 chs Exp $

KERNEL_SETS=		GENERIC

BUILD_KERNELS+=		INSTALL

snap_md_post:
.ifndef UPDATE
	cd ${.CURDIR}/../distrib && ${MAKE} cleandir
.endif
	cd ${.CURDIR}/../distrib && ${MAKE} depend && ${MAKE}
	cd ${.CURDIR}/../distrib/${MACHINE} && ${MAKE} release
	cd ${.CURDIR}/../distrib/notes && ${MAKE} release

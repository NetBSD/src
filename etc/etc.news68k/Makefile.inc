#	$NetBSD: Makefile.inc,v 1.3 2001/11/29 22:45:58 thorpej Exp $

KERNEL_SETS=		GENERIC GENERIC_TINY

BUILD_KERNELS=		INSTALL

snap_md_post:
.ifndef UPDATE
	cd ${.CURDIR}/../distrib && ${MAKE} cleandir
.endif
	cd ${.CURDIR}/../distrib && ${MAKE} depend && ${MAKE}
	cd ${.CURDIR}/../distrib/news68k && ${MAKE} release
	cd ${.CURDIR}/../distrib/notes && ${MAKE} release

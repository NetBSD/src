#	$NetBSD: Makefile.inc,v 1.3 2001/11/29 22:45:57 thorpej Exp $

KERNEL_SETS=		GENERIC

BUILD_KERNELS+=		INSTALL GENERIC_MD

INSTALLATION_DIRS+=	installation

snap_md_post:
.ifndef UPDATE
	cd ${.CURDIR}/../distrib && ${MAKE} cleandir
.endif
	cd ${.CURDIR}/../distrib && ${MAKE} depend && ${MAKE}
	cd ${.CURDIR}/../distrib/macppc && ${MAKE} release
	cd ${.CURDIR}/../distrib/notes && ${MAKE} release

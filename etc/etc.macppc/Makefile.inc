#	$NetBSD: Makefile.inc,v 1.1 2000/06/07 20:29:00 matt Exp $

BUILD_KERNELS+=		INSTALL
INSTALLATION_DIRS+=	installation

snap_md_post:
.ifndef UPDATE
	cd ${.CURDIR}/../distrib && ${MAKE} cleandir
.endif
	cd ${.CURDIR}/../distrib && ${MAKE} depend && ${MAKE}
	cd ${.CURDIR}/../distrib/macppc && ${MAKE} release
	cd ${.CURDIR}/../distrib/notes && ${MAKE} release

#	$NetBSD: Makefile.inc,v 1.2.4.2 2000/06/23 15:46:00 minoura Exp $

BUILD_KERNELS+=		INSTALL GENERIC_MD
INSTALLATION_DIRS+=	installation

snap_md_post:
.ifndef UPDATE
	cd ${.CURDIR}/../distrib && ${MAKE} cleandir
.endif
	cd ${.CURDIR}/../distrib && ${MAKE} depend && ${MAKE}
	cd ${.CURDIR}/../distrib/macppc && ${MAKE} release
	cd ${.CURDIR}/../distrib/notes && ${MAKE} release

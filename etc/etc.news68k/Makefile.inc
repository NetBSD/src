#	$NetBSD: Makefile.inc,v 1.1.4.2 2000/06/23 15:46:01 minoura Exp $

BUILD_KERNELS+=		INSTALL
INSTALLATION_DIRS+=	installation

snap_md_post:
.ifndef UPDATE
	cd ${.CURDIR}/../distrib && ${MAKE} cleandir
.endif
	cd ${.CURDIR}/../distrib && ${MAKE} depend && ${MAKE}
	cd ${.CURDIR}/../distrib/news68k && ${MAKE} release
	cd ${.CURDIR}/../distrib/notes && ${MAKE} release

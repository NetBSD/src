#	$NetBSD: Makefile.inc,v 1.1 2000/06/12 15:09:39 tsutsui Exp $

BUILD_KERNELS+=		INSTALL
INSTALLATION_DIRS+=	installation

snap_md_post:
.ifndef UPDATE
	cd ${.CURDIR}/../distrib && ${MAKE} cleandir
.endif
	cd ${.CURDIR}/../distrib && ${MAKE} depend && ${MAKE}
	cd ${.CURDIR}/../distrib/news68k && ${MAKE} release
	cd ${.CURDIR}/../distrib/notes && ${MAKE} release

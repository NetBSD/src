#	$NetBSD: Makefile.inc,v 1.2.6.2 2000/07/13 21:01:46 he Exp $

BUILD_KERNELS+=		INSTALL
EXTRA_KERNELS+=		GENERIC_MD
INSTALLATION_DIRS+=	installation \
			installation/floppy

snap_md_post:
.ifndef UPDATE
	cd ${.CURDIR}/../distrib && ${MAKE} cleandir
.endif
	cd ${.CURDIR}/../distrib && ${MAKE} depend && ${MAKE}
	cd ${.CURDIR}/../distrib/macppc && ${MAKE} release
	cd ${.CURDIR}/../sys/arch/macppc/stand && ${MAKE} release

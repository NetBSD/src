#	$NetBSD: Makefile.inc,v 1.5 2002/10/10 16:58:07 jdolecek Exp $

KERNEL_SETS=		GENERIC GENERIC.MP

BUILD_KERNELS+=		INSTALL GENERIC_MD

INSTALLATION_DIRS+=	installation/floppy

snap_md_post:
.ifndef UPDATE
	cd ${.CURDIR}/../distrib && ${MAKE} cleandir
.endif
	cd ${.CURDIR}/../distrib && ${MAKE} depend && ${MAKE}
	cd ${.CURDIR}/../distrib/macppc && ${MAKE} release
	cd ${.CURDIR}/../distrib/notes && ${MAKE} release

#	$NetBSD: Makefile.inc,v 1.2 2002/05/02 15:09:46 lukem Exp $

KERNEL_SETS=		GENERIC

BUILD_KERNELS=		INSTALL

INSTALLATION_DIRS+=	installation/floppy

snap_md_post:
.ifndef UPDATE
	cd ${.CURDIR}/../distrib && ${MAKE} cleandir
.endif
	cd ${.CURDIR}/../distrib && ${MAKE} depend && ${MAKE}
	cd ${.CURDIR}/../distrib/newsmips && ${MAKE} release
	cd ${.CURDIR}/../distrib/notes && ${MAKE} release

#	$NetBSD: Makefile.inc,v 1.3 2002/05/02 15:09:46 lukem Exp $

KERNEL_SETS=		GENERIC

BUILD_KERNELS=	 	RAMDISK

INSTALLATION_DIRS+=	installation/miniroot

snap_md_post:
.ifndef UPDATE
	cd ${.CURDIR}/../distrib && ${MAKE} cleandir
.endif
	cd ${.CURDIR}/../distrib && ${MAKE} depend && ${MAKE}
	cd ${.CURDIR}/../distrib/playstation2 && ${MAKE} release
	cd ${.CURDIR}/../distrib/notes && ${MAKE} release

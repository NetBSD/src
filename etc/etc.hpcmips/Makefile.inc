#	$NetBSD: Makefile.inc,v 1.5 2002/03/23 03:20:30 shin Exp $

KERNEL_SETS=		GENERIC TX3912

BUILD_KERNELS=		RAMDISK INSTALL_TX3912

snap_md_post:
.ifndef UPDATE
	cd ${.CURDIR}/../distrib && ${MAKE} cleandir
.endif
	cd ${.CURDIR}/../distrib && ${MAKE} depend && ${MAKE}
	cd ${.CURDIR}/../distrib/hpcmips && ${MAKE} release
	cd ${.CURDIR}/../distrib/notes && ${MAKE} release

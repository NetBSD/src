#	$NetBSD: Makefile.inc,v 1.4 2001/11/29 22:45:56 thorpej Exp $

KERNEL_SETS=		GENERIC TX3912 TX3922

BUILD_KERNELS=		RAMDISK INSTALL_TX3912 INSTALL_TX3922

snap_md_post:
.ifndef UPDATE
	cd ${.CURDIR}/../distrib && ${MAKE} cleandir
.endif
	cd ${.CURDIR}/../distrib && ${MAKE} depend && ${MAKE}
	cd ${.CURDIR}/../distrib/hpcmips && ${MAKE} release
	cd ${.CURDIR}/../distrib/notes && ${MAKE} release

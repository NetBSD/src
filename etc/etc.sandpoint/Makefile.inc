#	$NetBSD: Makefile.inc,v 1.5 2001/12/17 14:41:49 briggs Exp $
#
#	etc.sandpoint/Makefile.inc -- sandpoint-specific etc Makefile targets
#

KERNEL_SETS=	GENERIC

BUILD_KERNELS=  INSTALL

snap_md_post:
.ifndef UPDATE
	cd ${.CURDIR}/../distrib && ${MAKE} cleandir
.endif
	cd ${.CURDIR}/../distrib && ${MAKE} depend && ${MAKE}
	cd ${.CURDIR}/../distrib/sandpoint && ${MAKE} release
	cd ${.CURDIR}/../distrib/notes && ${MAKE} release

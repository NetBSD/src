#	$NetBSD: Makefile.inc,v 1.7 2002/05/16 19:29:36 briggs Exp $
#
#	etc.sandpoint/Makefile.inc -- sandpoint-specific etc Makefile targets
#

KERNEL_SETS=		GENERIC GENERIC.X2
KERNEL_SUFFIXES=	srec

BUILD_KERNELS=  	INSTALL

snap_md_post:
.ifndef UPDATE
	cd ${.CURDIR}/../distrib && ${MAKE} cleandir
.endif
	cd ${.CURDIR}/../distrib && ${MAKE} depend && ${MAKE}
	cd ${.CURDIR}/../distrib/sandpoint && ${MAKE} release
	cd ${.CURDIR}/../distrib/notes && ${MAKE} release

#	$NetBSD: Makefile.inc,v 1.5 2002/05/13 01:27:01 soren Exp $
#
#	etc.sgimips/Makefile.inc -- sgimips-specific etc Makefile targets
#

KERNEL_SETS=		GENERIC GENERIC_INDY

BUILD_KERNELS=		RAMDISK

INSTALLATION_DIRS+=	installation/netboot

snap_md_post:
.ifndef UPDATE
	cd ${.CURDIR}/../distrib && ${MAKE} cleandir
.endif
	cd ${.CURDIR}/../distrib && ${MAKE} depend && ${MAKE}
	cd ${.CURDIR}/../distrib/prep && ${MAKE} release
	cd ${.CURDIR}/../distrib/notes && ${MAKE} release

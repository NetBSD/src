#	$NetBSD: Makefile.inc,v 1.2 2001/11/29 22:46:00 thorpej Exp $
#
#	etc.sgimips/Makefile.inc -- sgimips-specific etc Makefile targets
#

KERNEL_SETS=		GENERIC GENERIC_INDY

BUILD_KERNELS=		RAMDISK

# Add any extra release directories to be made
INSTALLATION_DIRS+=installation/miniroot installation/netboot

snap_md_post:
.ifndef UPDATE
	cd ${.CURDIR}/../distrib && ${MAKE} cleandir
.endif
	cd ${.CURDIR}/../distrib && ${MAKE} depend && ${MAKE}
	cd ${.CURDIR}/../distrib/prep && ${MAKE} release
	cd ${.CURDIR}/../distrib/notes && ${MAKE} release

#	$NetBSD: Makefile.inc,v 1.6 2002/05/21 20:56:19 rafal Exp $
#
#	etc.sgimips/Makefile.inc -- sgimips-specific etc Makefile targets
#

KERNEL_SETS=		GENERIC GENERIC_INDY
KERNEL_SUFFIXES=	ecoff

#EXTRA_KERNELS=		NETBOOT

BUILD_KERNELS=		RAMDISK RAMDISK_INDY

INSTALLATION_DIRS+=	installation/netboot \
			installation/diskimage

snap_md_post: 
.ifndef UPDATE
	cd ${.CURDIR}/../distrib && ${MAKE} cleandir
.endif
	cd ${.CURDIR}/../distrib && ${MAKE} depend && ${MAKE}
	cd ${.CURDIR}/../distrib/sgimips && ${MAKE} release
	cd ${.CURDIR}/../distrib/notes && ${MAKE} release

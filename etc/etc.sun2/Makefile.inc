#	$NetBSD: Makefile.inc,v 1.3 2002/05/02 15:09:49 lukem Exp $
#
#	etc.sun2/Makefile.inc -- sun2-specific etc Makefile targets
#

BIN3+= etc.sun2/ttyaction

KERNEL_SETS=		GENERIC FOURMEG DISKLESS

EXTRA_KERNELS=		INSTALL

BUILD_KERNELS=		RAMDISK

INSTALLATION_DIRS+=	installation/miniroot	\
			installation/netboot	\
			installation/tapeimage

snap_md_post:
# Build the ramdisk image and patch it into the ramdisk kernels.
# Build the miniroot, etc. and copy all to the release area.
.ifndef UPDATE
	cd ${.CURDIR}/../distrib/sun2 && ${MAKE} clean
.endif
	cd ${.CURDIR}/../distrib/sun2 && \
	  ${MAKE} depend && ${MAKE} ${_J} && ${MAKE} release
	cd ${.CURDIR}/../distrib/notes && ${MAKE} && ${MAKE} release

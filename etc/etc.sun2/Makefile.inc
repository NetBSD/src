#	$NetBSD: Makefile.inc,v 1.2 2001/11/29 22:46:01 thorpej Exp $
#
#	etc.sun2/Makefile.inc -- sun2-specific etc Makefile targets
#

BIN3+= etc.sun2/ttyaction

KERNEL_SETS=		GENERIC FOURMEG DISKLESS

EXTRA_KERNELS=		INSTALL

BUILD_KERNELS=		RAMDISK

snap_md_post:
# Build the ramdisk image and patch it into the ramdisk kernels.
# Build the miniroot, etc. and copy all to the release area.
.ifndef UPDATE
	cd ${.CURDIR}/../distrib/sun2 && ${MAKE} clean
.endif
	cd ${.CURDIR}/../distrib/sun2 && \
	  ${MAKE} depend && ${MAKE} ${_J} && ${MAKE} release
	cd ${.CURDIR}/../distrib/notes && ${MAKE} && ${MAKE} release

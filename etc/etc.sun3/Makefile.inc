#	$NetBSD: Makefile.inc,v 1.12 2001/11/29 22:46:01 thorpej Exp $
#
#	etc.sun3/Makefile.inc -- sun3-specific etc Makefile targets
#

BIN3+= etc.sun3/ttyaction

KERNEL_SETS=		GENERIC GENERIC3X DISKLESS DISKLESS3X

EXTRA_KERNELS=		INSTALL INSTALL3X

BUILD_KERNELS=		RAMDISK RAMDISK3X

snap_md_post:
# Build the ramdisk image and patch it into the ramdisk kernels.
# Build the miniroot, etc. and copy all to the release area.
.ifndef UPDATE
	cd ${.CURDIR}/../distrib/sun3 && ${MAKE} clean
.endif
	cd ${.CURDIR}/../distrib/sun3 && \
	  ${MAKE} depend && ${MAKE} ${_J} && ${MAKE} release
	cd ${.CURDIR}/../distrib/notes && ${MAKE} && ${MAKE} release

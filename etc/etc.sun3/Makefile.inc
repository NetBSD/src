#	$NetBSD: Makefile.inc,v 1.11 2001/04/21 17:43:24 he Exp $
#
#	etc.sun3/Makefile.inc -- sun3-specific etc Makefile targets
#

BIN3+= etc.sun3/ttyaction

# These are put in ${RELEASEDIR} by "make release" in distrib/sun3
# after it builds and patches in the installation ramdisk image.
BUILD_KERNELS= \
 RAMDISK RAMDISK3X

# These are just built and copied to the release area.
EXTRA_KERNELS= \
 GENERIC3X \
 INSTALL INSTALL3X \
 DISKLESS DISKLESS3X

snap_md_post:
# Build the ramdisk image and patch it into the ramdisk kernels.
# Build the miniroot, etc. and copy all to the release area.
.ifndef UPDATE
	cd ${.CURDIR}/../distrib/sun3 && ${MAKE} clean
.endif
	cd ${.CURDIR}/../distrib/sun3 && \
	  ${MAKE} depend && ${MAKE} ${_J} && ${MAKE} release
	cd ${.CURDIR}/../distrib/notes && ${MAKE} && ${MAKE} release

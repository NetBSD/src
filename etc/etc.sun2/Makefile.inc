#	$NetBSD: Makefile.inc,v 1.1 2001/05/18 00:13:27 fredette Exp $
#
#	etc.sun2/Makefile.inc -- sun2-specific etc Makefile targets
#

BIN3+= etc.sun2/ttyaction

# These are put in ${RELEASEDIR} by "make release" in distrib/sun2
# after it builds and patches in the installation ramdisk image.
BUILD_KERNELS= \
 RAMDISK 

# These are just built and copied to the release area.
EXTRA_KERNELS= \
 INSTALL \
 FOURMEG \
 DISKLESS 

snap_md_post:
# Build the ramdisk image and patch it into the ramdisk kernels.
# Build the miniroot, etc. and copy all to the release area.
.ifndef UPDATE
	cd ${.CURDIR}/../distrib/sun2 && ${MAKE} clean
.endif
	cd ${.CURDIR}/../distrib/sun2 && \
	  ${MAKE} depend && ${MAKE} ${_J} && ${MAKE} release
	cd ${.CURDIR}/../distrib/notes && ${MAKE} && ${MAKE} release

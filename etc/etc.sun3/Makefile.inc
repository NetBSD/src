#	$NetBSD: Makefile.inc,v 1.9 1999/05/21 04:02:02 gwr Exp $
#
#	etc.sun3/Makefile.inc -- sun3-specific etc Makefile targets
#

BIN3+= etc.sun3/ttyaction

# These are put in ${RELEASEDIR} by "make release" in distrib/sun3
# after it builds and patches in the installation ramdisk image.
RAMDISK_KERNELS= \
 RAMDISK RAMDISK3X

# These are just built and copied to the release area.
OTHER_KERNELS= \
 GENERIC GENERIC3X \
 INSTALL INSTALL3X \
 DISKLESS DISKLESS3X

ALL_KERNELS = $(RAMDISK_KERNELS) $(OTHER_KERNELS)

# Build all the kernels, then copy all but ramdisk kernels
# to the release area.  Those are done by snap_md_post.
snap_kern:
.for configfile in ${ALL_KERNELS}
	cd ${KERNCONFDIR} && config \
		-b ${KERNOBJDIR}/${configfile} -s ${KERNSRCDIR} ${configfile}
.ifndef UPDATE
	cd ${KERNOBJDIR}/${configfile} && ${MAKE} clean 
.endif
	cd ${KERNOBJDIR}/${configfile} && ${MAKE} depend && ${MAKE} ${_J}
.endfor # EXTRA_KERNELS
.for configfile in ${OTHER_KERNELS}
	cd ${KERNOBJDIR}/${configfile} && gzip -c -9 < netbsd > \
		${RELEASEDIR}/binary/kernel/$${kernel}.${configfile}.gz
.endfor # EXTRA_KERNELS

# Build the ramdisk image and patch it into the ramdisk kernels.
# Build the miniroot, etc. and copy all to the release area.
snap_md_post:
.ifndef UPDATE
	cd ${.CURDIR}/../distrib/sun3 && ${MAKE} clean
.endif
	cd ${.CURDIR}/../distrib/sun3 && \
	  ${MAKE} depend && ${MAKE} ${_J} && ${MAKE} release


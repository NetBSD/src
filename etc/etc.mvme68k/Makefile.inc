#	$NetBSD: Makefile.inc,v 1.4 2002/03/31 16:07:55 bjh21 Exp $
#
#	etc.mvme68k/Makefile.inc -- mvme68k-specific etc Makefile targets
#

KERNEL_SETS=	GENERIC VME147 VME162 VME167 VME172 VME177

BUILD_KERNELS=	RAMDISK

INSTALLATION_DIRS += installation/miniroot
INSTALLATION_DIRS += installation/netboot
INSTALLATION_DIRS += installation/tapeimage

# Tar up extra kernel sets.
# Build the boot floppies and install them
snap_md_post:
.ifndef UPDATE
	cd ${.CURDIR}/../distrib && ${MAKE} cleandir
.endif
	cd ${.CURDIR}/../distrib && ${MAKE} depend && ${MAKE}
	cd ${.CURDIR}/../distrib/mvme68k && ${MAKE} release
	cd ${.CURDIR}/../distrib/miniroot && ${MAKE} release
	cd ${.CURDIR}/../distrib/notes && ${MAKE} release
	cp -p ${DESTDIR}/usr/mdec/sboot ${RELEASEDIR}/installation/netboot
	cp -p ${DESTDIR}/usr/mdec/netboot ${RELEASEDIR}/installation/netboot
	cp -p ${DESTDIR}/usr/mdec/stboot ${RELEASEDIR}/installation/tapeimage
	cp -p ${DESTDIR}/usr/mdec/bootst ${RELEASEDIR}/installation/tapeimage
	${MAKESUMS} -t ${RELEASEDIR}/installation/netboot -all
	${MAKESUMS} -t ${RELEASEDIR}/installation/tapeimage -all


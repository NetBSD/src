#	$NetBSD: Makefile.inc,v 1.1 2000/11/24 19:49:40 scw Exp $
#
#	etc.mvme68k/Makefile.inc -- mvme68k-specific etc Makefile targets
#

# These are packaged and distributed
EXTRA_KERNELS+= VME147 VME162 VME167 VME172 VME177

# These are for use by the bootfloppy construction tools
BUILD_KERNELS+= RAMDISK
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
	cd ${.CURDIR}/../distrib/notes && ${MAKE} release
	cp -p ${DESTDIR}/usr/mdec/sboot ${RELEASEDIR}/installation/netboot
	cp -p ${DESTDIR}/usr/mdec/netboot ${RELEASEDIR}/installation/netboot
	cp -p ${DESTDIR}/usr/mdec/stboot ${RELEASEDIR}/installation/tapeimage
	cp -p ${DESTDIR}/usr/mdec/bootst ${RELEASEDIR}/installation/tapeimage
	sh ../distrib/sets/makesums -t ${RELEASEDIR}/installation/miniroot
	sh ../distrib/sets/makesums -t ${RELEASEDIR}/installation/netboot
	sh ../distrib/sets/makesums -t ${RELEASEDIR}/installation/tapeimage


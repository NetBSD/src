#	$NetBSD: Makefile.inc,v 1.6 2002/12/02 13:18:09 lukem Exp $
#
#	etc.mvme68k/Makefile.inc -- mvme68k-specific etc Makefile targets
#

KERNEL_SETS=		GENERIC VME147 VME162 VME167 VME172 VME177

BUILD_KERNELS=		RAMDISK

INSTALLATION_DIRS+=	installation/miniroot	\
			installation/netboot	\
			installation/tapeimage

snap_md_post:
	cp -p ${DESTDIR}/usr/mdec/sboot ${RELEASEDIR}/installation/netboot
	cp -p ${DESTDIR}/usr/mdec/netboot ${RELEASEDIR}/installation/netboot
	cp -p ${DESTDIR}/usr/mdec/stboot ${RELEASEDIR}/installation/tapeimage
	cp -p ${DESTDIR}/usr/mdec/bootst ${RELEASEDIR}/installation/tapeimage
	${MAKESUMS} -t ${RELEASEDIR}/installation/netboot -all
	${MAKESUMS} -t ${RELEASEDIR}/installation/tapeimage -all


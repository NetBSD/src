#	$NetBSD: Makefile.inc,v 1.10 2008/03/18 04:20:37 lukem Exp $
#
#	etc.mvme68k/Makefile.inc -- mvme68k-specific etc Makefile targets
#

# If you change the list of distributed kernels, don't forget
# to update the release documentation in distrib/notes/common/contents

KERNEL_SETS=		GENERIC VME147 VME162 VME167 VME172 VME177

BUILD_KERNELS=		RAMDISK

INSTALLATION_DIRS+=	installation/miniroot	\
			installation/netboot	\
			installation/tapeimage

snap_md_post:
.for file in netboot sboot
	${HOST_INSTALL_FILE} -m ${NONBINMODE} ${DESTDIR}/usr/mdec/${file} \
	    ${RELEASEDIR}/${RELEASEMACHINEDIR}/installation/netboot
.endfor
.for file in bootst stboot
	${HOST_INSTALL_FILE} -m ${NONBINMODE} ${DESTDIR}/usr/mdec/${file} \
	    ${RELEASEDIR}/${RELEASEMACHINEDIR}/installation/tapeimage
.endfor
	${MAKESUMS} -t ${RELEASEDIR}/${RELEASEMACHINEDIR}/installation/netboot -a
	${MAKESUMS} -t ${RELEASEDIR}/${RELEASEMACHINEDIR}/installation/tapeimage -a


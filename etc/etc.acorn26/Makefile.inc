#	$NetBSD: Makefile.inc,v 1.9.10.1 2008/03/24 07:14:41 keiichi Exp $
#
#	etc.acorn26/Makefile.inc -- acorn26-specific etc Makefile targets
#

# If you change the list of distributed kernels, don't forget
# to update the release documentation in distrib/notes/common/contents

KERNEL_SETS=		GENERIC FOURMEG

BUILD_KERNELS=		INSTALL

INSTALLATION_DIRS+=	installation/misc

snap_md_post:
	# install bootloader into installation directory
	${HOST_INSTALL_FILE} -m ${NONBINMODE} ${DESTDIR}/usr/mdec/boot26,ffa \
	    ${RELEASEDIR}/${RELEASEMACHINEDIR}/installation/misc

	# compute checksums
	${MAKESUMS} -t ${RELEASEDIR}/${RELEASEMACHINEDIR}/installation/misc 'boot26,ffa'

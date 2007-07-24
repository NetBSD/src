#	$NetBSD: Makefile.inc,v 1.9 2007/07/24 10:49:23 pavel Exp $
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
	    ${RELEASEDIR}/${MACHINE}/installation/misc

	# compute checksums
	${MAKESUMS} -t ${RELEASEDIR}/${MACHINE}/installation/misc 'boot26,ffa'

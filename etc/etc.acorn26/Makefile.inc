#	$NetBSD: Makefile.inc,v 1.8 2003/03/01 15:36:22 bjh21 Exp $
#
#	etc.acorn26/Makefile.inc -- acorn26-specific etc Makefile targets
#

KERNEL_SETS=		GENERIC FOURMEG

BUILD_KERNELS=		INSTALL

INSTALLATION_DIRS+=	installation/misc

snap_md_post:
	# install bootloader into installation directory
	${HOST_INSTALL_FILE} -m ${NONBINMODE} ${DESTDIR}/usr/mdec/boot26,ffa \
	    ${RELEASEDIR}/${MACHINE}/installation/misc

	# compute checksums
	${MAKESUMS} -t ${RELEASEDIR}/${MACHINE}/installation/misc 'boot26,ffa'

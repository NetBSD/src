#	$NetBSD: Makefile.inc,v 1.7 2003/01/03 15:34:36 lukem Exp $
#
#	etc.acorn26/Makefile.inc -- acorn26-specific etc Makefile targets
#

KERNEL_SETS=		GENERIC FOURMEG

BUILD_KERNELS=		INSTALL

INSTALLATION_DIRS+=	installation/misc

snap_md_post:
	# install bootloader into installation directory
	cp -p ${DESTDIR}/usr/mdec/boot26,ffa \
	    ${RELEASEDIR}/${MACHINE}/installation/misc

	# compute checksums
	${MAKESUMS} -t ${RELEASEDIR}/${MACHINE}/installation/misc 'boot26,ffa'

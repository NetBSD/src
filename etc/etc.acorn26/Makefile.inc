#	$NetBSD: Makefile.inc,v 1.6 2002/12/02 13:18:05 lukem Exp $
#
#	etc.acorn26/Makefile.inc -- acorn26-specific etc Makefile targets
#

KERNEL_SETS=		GENERIC FOURMEG

BUILD_KERNELS=		INSTALL

INSTALLATION_DIRS+=	installation/misc

snap_md_post:
	# install bootloader into installation directory
	cp -p ${DESTDIR}/usr/mdec/boot26,ffa ${RELEASEDIR}/installation/misc/.

	# compute checksums
	${MAKESUMS} -t ${RELEASEDIR}/installation/misc 'boot26,ffa'

#	$NetBSD: Makefile.inc,v 1.5 2002/05/02 15:09:41 lukem Exp $
#
#	etc.acorn26/Makefile.inc -- acorn26-specific etc Makefile targets
#

KERNEL_SETS=		GENERIC FOURMEG

BUILD_KERNELS=		INSTALL

INSTALLATION_DIRS+=	installation/misc

snap_md_post:
.ifndef UPDATE
	cd ${.CURDIR}/../distrib && ${MAKE} cleandir
.endif
	cd ${.CURDIR}/../distrib && ${MAKE} depend && ${MAKE}
	cd ${.CURDIR}/../distrib/notes && ${MAKE} release
	cd ${.CURDIR}/../distrib/acorn26 && ${MAKE} release

	# install bootloader into installation directory
	cp -p ${DESTDIR}/usr/mdec/boot26,ffa ${RELEASEDIR}/installation/misc/.

	# compute checksums
	${MAKESUMS} -t ${RELEASEDIR}/installation/misc 'boot26,ffa'

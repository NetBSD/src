#	$NetBSD: Makefile.inc,v 1.3 2002/04/01 19:12:16 bjh21 Exp $
#
#	etc.acorn26/Makefile.inc -- acorn26-specific etc Makefile targets
#

KERNEL_SETS=		GENERIC FOURMEG

INSTALLATION_DIRS+=	installation/misc

snap_md_post:
.ifndef UPDATE
	cd ${.CURDIR}/../distrib && ${MAKE} cleandir
.endif
	cd ${.CURDIR}/../distrib && ${MAKE} depend && ${MAKE}
	cd ${.CURDIR}/../distrib/notes && ${MAKE} release

	# install bootloader into installation directory
	cp -p ${DESTDIR}/usr/mdec/boot26,ffa ${RELEASEDIR}/installation/misc/.

	# compute checksums
	${MAKESUMS} -t ${RELEASEDIR}/installation/misc 'boot26,ffa'

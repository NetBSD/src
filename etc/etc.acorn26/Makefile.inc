#	$NetBSD: Makefile.inc,v 1.1 2002/03/24 21:20:39 bjh21 Exp $
#
#	etc.acorn26/Makefile.inc -- acorn26-specific etc Makefile targets
#

KERNEL_SETS=		GENERIC FOURMEG

snap_md_post:
.ifndef UPDATE
	cd ${.CURDIR}/../distrib && ${MAKE} cleandir
.endif
	cd ${.CURDIR}/../distrib && ${MAKE} depend && ${MAKE}
	cd ${.CURDIR}/../distrib/notes && ${MAKE} release

	# install bootloader into installation directory
	cp -p ${DESTDIR}/usr/mdec/boot26,ffa ${RELEASEDIR}/installation/.

	# compute checksums
	sh ${.CURDIR}/../distrib/sets/makesums -t ${RELEASEDIR}/installation '*'

#	$NetBSD: Makefile.inc,v 1.2 2002/05/17 16:16:14 toshii Exp $

KERNEL_SETS=		IPAQ JORNADA720

snap_md_post:
.ifndef UPDATE
	cd ${.CURDIR}/../distrib && ${MAKE} cleandir
.endif
	cd ${.CURDIR}/../distrib && ${MAKE} depend && ${MAKE}
	cd ${.CURDIR}/../distrib/hpcarm && ${MAKE} release
	cd ${.CURDIR}/../distrib/notes && ${MAKE} release

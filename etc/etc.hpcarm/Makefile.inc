#	$NetBSD: Makefile.inc,v 1.1 2002/04/13 00:07:25 toshii Exp $

KERNEL_SETS=		IPAQ JORNADA720

snap_md_post:
.ifndef UPDATE
	cd ${.CURDIR}/../distrib && ${MAKE} cleandir
.endif
	cd ${.CURDIR}/../distrib && ${MAKE} depend && ${MAKE}
	cd ${.CURDIR}/../distrib/hpcmips && ${MAKE} release
	cd ${.CURDIR}/../distrib/notes && ${MAKE} release

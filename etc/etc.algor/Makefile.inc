#	$NetBSD: Makefile.inc,v 1.1 2002/04/01 23:32:17 thorpej Exp $
#
#	etc.algor/Makefile.inc -- algor-specific etc Makefile targets
#

KERNEL_SETS=		P4032 P5064

snap_md_post:
.ifndef UPDATE
	cd ${.CURDIR}/../distrib && ${MAKE} cleandir
.endif
	cd ${.CURDIR}/../distrib && ${MAKE} depend && ${MAKE}
#	cd ${.CURDIR}/../distrib/algor && ${MAKE} release
	cd ${.CURDIR}/../distrib/notes && ${MAKE} release

#	$NetBSD: Makefile.inc,v 1.1 2002/02/27 21:17:52 scw Exp $
#
#	etc.mvmeppc/Makefile.inc -- mvmeppc-specific etc Makefile targets
#

KERNEL_SETS=		MVME1600

snap_md_post:
.ifndef UPDATE
	cd ${.CURDIR}/../distrib && ${MAKE} cleandir
.endif
	cd ${.CURDIR}/../distrib && ${MAKE} depend && ${MAKE}
	cd ${.CURDIR}/../distrib/mvmeppc && ${MAKE} release
	cd ${.CURDIR}/../distrib/notes && ${MAKE} release

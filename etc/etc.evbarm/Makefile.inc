#	$NetBSD: Makefile.inc,v 1.5 2002/03/28 22:18:23 thorpej Exp $
#
#	etc.evbarm/Makefile.inc -- evbarm-specific etc Makefile targets
#

KERNEL_SETS=		INTEGRATOR IQ80310 IQ80321
KERNEL_SUFFIXES=	bin srec

snap_md_post:
.ifndef UPDATE
	cd ${.CURDIR}/../distrib && ${MAKE} cleandir
.endif
	cd ${.CURDIR}/../distrib && ${MAKE} depend && ${MAKE}
	cd ${.CURDIR}/../distrib/evbarm && ${MAKE} release
	cd ${.CURDIR}/../distrib/notes && ${MAKE} release

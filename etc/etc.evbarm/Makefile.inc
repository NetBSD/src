#	$NetBSD: Makefile.inc,v 1.6 2002/03/29 20:20:24 thorpej Exp $
#
#	etc.evbarm/Makefile.inc -- evbarm-specific etc Makefile targets
#

KERNEL_SETS=		INTEGRATOR IQ80310 IQ80321
KERNEL_SUFFIXES=	bin srec

INSTALLATION_DIRS+=	binary/gzimg

snap_md_post:
.ifndef UPDATE
	cd ${.CURDIR}/../distrib && ${MAKE} cleandir
.endif
	cd ${.CURDIR}/../distrib && ${MAKE} depend && ${MAKE}
	cd ${.CURDIR}/../distrib/evbarm && ${MAKE} release
	cd ${.CURDIR}/../distrib/notes && ${MAKE} release

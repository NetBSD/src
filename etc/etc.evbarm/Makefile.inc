#	$NetBSD: Makefile.inc,v 1.8 2002/04/13 00:51:57 thorpej Exp $
#
#	etc.evbarm/Makefile.inc -- evbarm-specific etc Makefile targets
#

KERNEL_SETS=		INTEGRATOR IQ80310 IQ80321 TEAMASA_NPWR
KERNEL_SUFFIXES=	bin srec

BUILD_KERNELS+=		TEAMASA_NPWR_INSTALL

INSTALLATION_DIRS+=	binary/gzimg				\
			installation/instkernel

snap_md_post:
.ifndef UPDATE
	cd ${.CURDIR}/../distrib && ${MAKE} cleandir
.endif
	cd ${.CURDIR}/../distrib && ${MAKE} depend && ${MAKE}
	cd ${.CURDIR}/../distrib/evbarm && ${MAKE} release
	cd ${.CURDIR}/../distrib/notes && ${MAKE} release

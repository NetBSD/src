#	$NetBSD: Makefile.inc,v 1.11 2002/03/04 16:43:06 shiba Exp $
#
#	mac68k-specific release building goo
#

KERNEL_SETS=		GENERIC GENERICSBC

BUILD_KERNELS=		INSTALL INSTALLSBC

INSTALLATION_DIRS+=	installation/instkernel

snap_md_post:
.ifndef UPDATE
	cd ${.CURDIR}/../distrib && ${MAKE} cleandir
.endif
	cd ${.CURDIR}/../distrib && ${MAKE} depend && ${MAKE}
	cd ${.CURDIR}/../distrib/mac68k && ${MAKE} release
	cd ${.CURDIR}/../distrib/notes && ${MAKE} release
	sh ../distrib/sets/makesums -t ${RELEASEDIR}/installation/instkernel '*.gz'

#	$NetBSD: Makefile.inc,v 1.13 2002/05/02 15:09:44 lukem Exp $
#
#	mac68k-specific release building goo
#

KERNEL_SETS=		GENERIC GENERICSBC

BUILD_KERNELS=		INSTALL INSTALLSBC

INSTALLATION_DIRS+=	installation/miniroot	\
			installation/instkernel

snap_md_post:
.ifndef UPDATE
	cd ${.CURDIR}/../distrib && ${MAKE} cleandir
.endif
	cd ${.CURDIR}/../distrib && ${MAKE} depend && ${MAKE}
	cd ${.CURDIR}/../distrib/mac68k && ${MAKE} release
	cd ${.CURDIR}/../distrib/notes && ${MAKE} release
	${MAKESUMS} -t ${RELEASEDIR}/installation/instkernel '*.gz'

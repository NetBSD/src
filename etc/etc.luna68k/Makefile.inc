#	$NetBSD: Makefile.inc,v 1.1 2002/05/08 13:50:22 jdolecek Exp $
#
#	etc.luna68k/Makefile.inc -- luna68k-specific etc Makefile targets
#

KERNEL_SETS=		GENERIC

INSTALLATION_DIRS+=	installation/netboot

snap_md_post:
.ifndef UPDATE
	cd ${.CURDIR}/../distrib && ${MAKE} cleandir
.endif
	cd ${.CURDIR}/../distrib && ${MAKE} depend && ${MAKE}
	cd ${.CURDIR}/../distrib/prep && ${MAKE} release
	cd ${.CURDIR}/../distrib/notes && ${MAKE} release

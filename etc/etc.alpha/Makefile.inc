#	$NetBSD: Makefile.inc,v 1.1.10.2 2000/07/13 17:18:16 he Exp $

BUILD_KERNELS+=		INSTALL
INSTALLATION_DIRS+=	installation/floppy	\
			installation/diskimage	\
			installation/instkernel	\
			installation/netboot	\
			installation/tapeimage	\
			installation/misc

snap_md_post:
.ifndef UPDATE
	cd ${.CURDIR}/../distrib && ${MAKE} cleandir
.endif
	cd ${.CURDIR}/../distrib && ${MAKE} depend && ${MAKE}
	cd ${.CURDIR}/../distrib/alpha && ${MAKE} release
	cd ${.CURDIR}/../sys/arch/alpha/stand/netboot && ${MAKE} release

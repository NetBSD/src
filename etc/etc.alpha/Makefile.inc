#	$NetBSD: Makefile.inc,v 1.1 1999/06/30 18:39:37 ross Exp $

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
	cd ${.CURDIR}/../distrib/notes && ${MAKE} release

#	$NetBSD: Makefile.inc,v 1.5 2002/10/10 16:58:06 jdolecek Exp $

KERNEL_SETS=		GENERIC GENERIC.MP

BUILD_KERNELS+=		INSTALL

INSTALLATION_DIRS+=	installation/floppy	\
			installation/diskimage	\
			installation/instkernel	\
			installation/netboot	\
			installation/misc

snap_md_post:
.ifndef UPDATE
	cd ${.CURDIR}/../distrib && ${MAKE} cleandir
.endif
	cd ${.CURDIR}/../distrib && ${MAKE} depend && ${MAKE}
	cd ${.CURDIR}/../distrib/alpha && ${MAKE} release
	cd ${.CURDIR}/../distrib/notes && ${MAKE} release
	cd ${.CURDIR}/../sys/arch/alpha/stand/netboot && ${MAKE} release

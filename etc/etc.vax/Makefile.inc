#	$NetBSD: Makefile.inc,v 1.11 2002/03/29 04:45:38 matt Exp $
#
#	etc.vax/Makefile.inc -- vax-specific etc Makefile targets
#

KERNEL_SETS=	GENERIC
BUILD_KERNELS=	INSTALL

UUDECODE?=      uudecode

distribution: pcs750.bin

pcs750.bin: ${.CURDIR}/etc.vax/pcs750.bin.uu
	${UUDECODE} ${.ALLSRC}

CLEANFILES+=	pcs750.bin
INSTALLATION_DIRS+=	installation \
			installation/bootfs \
			installation/netboot

snap_md_post:
.ifndef UPDATE
	cd ${.CURDIR}/../distrib && ${MAKE} cleandir
.endif
	cd ${.CURDIR}/../distrib && ${MAKE} depend && ${MAKE}
	cd ${.CURDIR}/../distrib/vax && ${MAKE} release
	cd ${.CURDIR}/../distrib/notes && ${MAKE} release

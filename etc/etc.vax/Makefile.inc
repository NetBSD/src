#	$NetBSD: Makefile.inc,v 1.12 2002/03/29 22:14:20 matt Exp $
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

iso-image_md_pre:
	cp -f ${RELEASEDIR}/installation/netboot/netbsd.ram.gz \
		${RELEASEDIR}/netbsd
	cp -f ${DESTDIR}/usr/mdec/boot ${RELEASEDIR}

iso-image_md_post:
.if ${USETOOLS} == "yes"
	${TOOLDIR}/bin/nbvax-installboot \
		${RELEASEDIR}/installation/cdrom/netbsd-${MACHINE}.iso \
		${DESTDIR}/usr/mdec/xxboot
.else
	${DESTDIR}/usr/mdec/installboot \
		${RELEASEDIR}/installation/cdrom/netbsd-${MACHINE}.iso \
		${DESTDIR}/usr/mdec/xxboot
.endif

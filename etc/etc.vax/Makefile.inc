#	$NetBSD: Makefile.inc,v 1.13 2002/04/09 18:06:45 matt Exp $
#
#	etc.vax/Makefile.inc -- vax-specific etc Makefile targets
#

KERNEL_SETS=	GENERIC
BUILD_KERNELS=	INSTALL

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
	${INSTALLBOOT} -m ${MACHINE} \
		${RELEASEDIR}/installation/cdrom/netbsd-${MACHINE}.iso \
		${DESTDIR}/usr/mdec/xxboot

#	$NetBSD: Makefile.inc,v 1.16 2002/12/02 13:18:14 lukem Exp $
#
#	etc.vax/Makefile.inc -- vax-specific etc Makefile targets
#

KERNEL_SETS=		GENERIC GENERIC.MP

BUILD_KERNELS=		INSTALL

INSTALLATION_DIRS+=	installation/bootfs	\
			installation/miniroot	\
			installation/netboot

iso-image_md_pre:
	cp -f ${RELEASEDIR}/installation/netboot/netbsd.ram.gz \
		${RELEASEDIR}/netbsd
	cp -f ${DESTDIR}/usr/mdec/boot ${RELEASEDIR}

iso-image_md_post:
	${INSTALLBOOT} -m ${MACHINE} \
		${RELEASEDIR}/installation/cdrom/netbsd-${MACHINE}.iso \
		${DESTDIR}/usr/mdec/xxboot

#	$NetBSD: Makefile.inc,v 1.17 2003/01/03 15:34:40 lukem Exp $
#
#	etc.vax/Makefile.inc -- vax-specific etc Makefile targets
#

KERNEL_SETS=		GENERIC GENERIC.MP

BUILD_KERNELS=		INSTALL

INSTALLATION_DIRS+=	installation/bootfs	\
			installation/miniroot	\
			installation/netboot

iso-image_md_pre:
	cp -f ${RELEASEDIR}/${MACHINE}/installation/netboot/netbsd.ram.gz \
	    ${RELEASEDIR}/${MACHINE}/netbsd
	cp -f ${DESTDIR}/usr/mdec/boot ${RELEASEDIR}/${MACHINE}

iso-image_md_post:
	${INSTALLBOOT} -m ${MACHINE} \
	    ${RELEASEDIR}/${MACHINE}/installation/cdrom/netbsd-${MACHINE}.iso \
	    ${DESTDIR}/usr/mdec/xxboot

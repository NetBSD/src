#	$NetBSD: Makefile.inc,v 1.18 2003/01/04 15:49:32 lukem Exp $
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
	    ${CDROM.work}/netbsd
	cp -f ${DESTDIR}/usr/mdec/boot ${CDROM.work}

iso-image_md_post:
	${INSTALLBOOT} -m ${MACHINE} ${CDROM.image} ${DESTDIR}/usr/mdec/xxboot

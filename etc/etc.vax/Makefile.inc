#	$NetBSD: Makefile.inc,v 1.20 2003/07/10 10:34:19 lukem Exp $
#
#	etc.vax/Makefile.inc -- vax-specific etc Makefile targets
#

KERNEL_SETS=		GENERIC GENERIC.MP

BUILD_KERNELS=		INSTALL

INSTALLATION_DIRS+=	installation/bootfs	\
			installation/miniroot	\
			installation/netboot

iso-image-md-pre:
	cp -f ${RELEASEDIR}/${MACHINE}/installation/netboot/netbsd.ram.gz \
	    ${CDROM.work}/netbsd
	cp -f ${DESTDIR}/usr/mdec/boot ${CDROM.work}

iso-image-md-post:
	${TOOL_INSTALLBOOT} -m ${MACHINE} ${CDROM.image} ${DESTDIR}/usr/mdec/xxboot

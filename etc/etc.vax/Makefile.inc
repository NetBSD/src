#	$NetBSD: Makefile.inc,v 1.19 2003/03/08 00:22:30 lukem Exp $
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
	${INSTALLBOOT} -m ${MACHINE} ${CDROM.image} ${DESTDIR}/usr/mdec/xxboot

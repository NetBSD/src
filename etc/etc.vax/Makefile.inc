#	$NetBSD: Makefile.inc,v 1.21 2004/03/23 22:35:30 matt Exp $
#
#	etc.vax/Makefile.inc -- vax-specific etc Makefile targets
#

KERNEL_SETS=		GENERIC GENERIC.MP

BUILD_KERNELS=		INSTALL

INSTALLATION_DIRS+=	installation/bootfs	\
			installation/miniroot	\
			installation/netboot

iso-image-md-pre:
	cp -f ${RELEASEDIR}/${MACHINE}/installation/netboot/install.ram.gz \
	    ${CDROM.dir}/netbsd
	cp -f ${DESTDIR}/usr/mdec/boot ${CDROM.dir}

iso-image-md-post:
	${TOOL_INSTALLBOOT} -m ${MACHINE} ${CDROM.image} ${DESTDIR}/usr/mdec/xxboot

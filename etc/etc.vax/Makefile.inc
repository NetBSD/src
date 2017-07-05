#	$NetBSD: Makefile.inc,v 1.24.34.1 2017/07/05 20:13:36 snj Exp $
#
#	etc.vax/Makefile.inc -- vax-specific etc Makefile targets
#

# If you change the list of distributed kernels, don't forget
# to update the release documentation in distrib/notes/common/contents

KERNEL_SETS=		GENERIC GENERIC.MP GENERIC_USB

BUILD_KERNELS=		INSTALL

INSTALLATION_DIRS+=	installation/bootfs	\
			installation/miniroot	\
			installation/netboot

iso-image-md-pre:
	cp -f ${RELEASEDIR}/${RELEASEMACHINEDIR}/installation/netboot/install.ram.gz \
	    ${CDROM.dir}/netbsd
	cp -f ${DESTDIR}/usr/mdec/boot ${CDROM.dir}

iso-image-md-post:
	${TOOL_INSTALLBOOT} -m ${MACHINE} ${CDROM_IMAGE} ${DESTDIR}/usr/mdec/xxboot

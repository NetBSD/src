#	$NetBSD: Makefile.inc,v 1.10 2004/11/13 10:27:14 sekiya Exp $
#
#	etc.sgimips/Makefile.inc -- sgimips-specific etc Makefile targets
#

KERNEL_SETS=		GENERIC32_IP12 GENERIC32_IP2x GENERIC32_IP3x
KERNEL_SUFFIXES=	ecoff

#EXTRA_KERNELS=		NETBOOT

BUILD_KERNELS=		INSTALL32_IP2x INSTALL32_IP3x

INSTALLATION_DIRS+=	installation/netboot \
			installation/diskimage

iso-image-md-pre:
	gzip -d -c ${RELEASEDIR}/${MACHINE}/binary/kernel/netbsd-INSTALL32_IP2x.gz > ${RELEASEDIR}/ip2x
	gzip -d -c ${RELEASEDIR}/${MACHINE}/binary/kernel/netbsd-INSTALL32_IP3x.gz > ${RELEASEDIR}/ip3x
	echo "ip2x=${RELEASEDIR}/ip2x" >> ${CDROM.pathlist}
	echo "ip3x=${RELEASEDIR}/ip3x" >> ${CDROM.pathlist}

SGI.image= ${RELEASEDIR}/${MACHINE}/installation/cdrom/netbsd-sgimips.img

iso-image-md-post:
	dd if=/dev/zero of=${SGI.image} bs=1m count=120
	vnconfig vnd0 ${SGI.image}
	${TOOLDIR}/bin/nbsgivol -f -i vnd0
	${TOOLDIR}/bin/nbsgivol -f -w aoutboot ${DESTDIR}/usr/mdec/aoutboot vnd0
	${TOOLDIR}/bin/nbsgivol -f -w ip2xboot ${DESTDIR}/usr/mdec/ip2xboot vnd0
	${TOOLDIR}/bin/nbsgivol -f -w ip3xboot ${DESTDIR}/usr/mdec/ip3xboot vnd0
	vnconfig -u vnd0
	dd if=${CDROM.image} of=${SGI.image} bs=512 seek=3135
	mv ${SGI.image} ${CDROM.image}
	rm -f ${RELEASEDIR}/ip2x ${RELEASEDIR}/ip3x

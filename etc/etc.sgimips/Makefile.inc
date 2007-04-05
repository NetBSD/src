#	$NetBSD: Makefile.inc,v 1.9.4.2 2007/04/05 20:48:36 bouyer Exp $
#
#	etc.sgimips/Makefile.inc -- sgimips-specific etc Makefile targets
#

KERNEL_SETS=		GENERIC32_IP2x GENERIC32_IP3x
KERNEL_SUFFIXES=	ecoff

#EXTRA_KERNELS=		NETBOOT

BUILD_KERNELS=		INSTALL32_IP2x INSTALL32_IP3x

INSTALLATION_DIRS+=	installation/netboot \
			installation/diskimage

iso-image-md-pre:
	gzip -d -c ${RELEASEDIR}/${MACHINE}/binary/kernel/netbsd-INSTALL32_IP2x.gz > ${RELEASEDIR}/ip2x
	gzip -d -c ${RELEASEDIR}/${MACHINE}/binary/kernel/netbsd-INSTALL32_IP3x.gz > ${RELEASEDIR}/ip3x
	echo "ip2x/=${RELEASEDIR}/ip2x/" >> ${CDROM.pathlist}
	echo "ip3x/=${RELEASEDIR}/ip3x/" >> ${CDROM.pathlist}

SGI.image= ${RELEASEDIR}/${MACHINE}/installation/cdrom/netbsd-sgimips.img

iso-image-md-post:
	dd if=/dev/zero of=${SGI.image} bs=1024k count=120
	${TOOLDIR}/bin/nbsgivol -f -i ${SGI.image}
	${TOOLDIR}/bin/nbsgivol -f -w aoutboot ${DESTDIR}/usr/mdec/aoutboot \
		${SGI.image}
	${TOOLDIR}/bin/nbsgivol -f -w ip2xboot ${DESTDIR}/usr/mdec/ip2xboot \
		${SGI.image}
	${TOOLDIR}/bin/nbsgivol -f -w ip3xboot ${DESTDIR}/usr/mdec/ip3xboot \
		${SGI.image}
# Don't hardcode this. Pick it up from bootblock.h
	SEEK=`egrep SGI_BOOT_BLOCK_SIZE_VOLHDR \
		${.CURDIR}/../../sys/sys/bootblock.h | sed -e 's/[^0-9]*//'`; \
		dd if=${CDROM.image} of=${SGI.image} bs=512 seek=$${SEEK}
	mv ${SGI.image} ${CDROM.image}
	rm -f ${RELEASEDIR}/ip2x ${RELEASEDIR}/ip3x

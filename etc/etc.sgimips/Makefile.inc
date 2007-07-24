#	$NetBSD: Makefile.inc,v 1.16 2007/07/24 10:49:28 pavel Exp $
#
#	etc.sgimips/Makefile.inc -- sgimips-specific etc Makefile targets
#

# If you change the list of distributed kernels, don't forget
# to update the release documentation in distrib/notes/common/contents

KERNEL_SETS=		GENERIC32_IP12 GENERIC32_IP2x GENERIC32_IP3x
KERNEL_SUFFIXES=	ecoff

#EXTRA_KERNELS=		NETBOOT

BUILD_KERNELS=		INSTALL32_IP2x INSTALL32_IP3x

INSTALLATION_DIRS+=	installation/netboot \
			installation/diskimage

iso-image-md-pre:
	cp ${RELEASEDIR}/${MACHINE}/binary/kernel/netbsd-INSTALL32_IP2x.gz ${RELEASEDIR}/ip2x
	cp ${RELEASEDIR}/${MACHINE}/binary/kernel/netbsd-INSTALL32_IP3x.gz ${RELEASEDIR}/ip3x
	echo "ip2x/=${RELEASEDIR}/ip2x/" >> ${CDROM.pathlist}
	echo "ip3x/=${RELEASEDIR}/ip3x/" >> ${CDROM.pathlist}

SGI.image= ${RELEASEDIR}/${MACHINE}/installation/cdrom/netbsd-sgimips.img

iso-image-md-post:
	@SEEK=`awk '/SGI_BOOT_BLOCK_SIZE_VOLHDR/{print $$3}' \
		${DESTDIR}/usr/include/sys/bootblock.h` ; \
	EST=`ls -l ${CDROM.image} \
		| awk '{ s += $$5} END { print int(('$${SEEK}'*512+s)/1024/1024/32+1)*32 }'` ; \
	echo "Estimated image size: $${EST} MB"; \
	dd if=/dev/zero of=${SGI.image} bs=1024k count=$${EST}; \
	${TOOLDIR}/bin/nbsgivol -f -i ${SGI.image}; \
	${TOOLDIR}/bin/nbsgivol -f -w aoutboot ${DESTDIR}/usr/mdec/aoutboot \
		${SGI.image}; \
	${TOOLDIR}/bin/nbsgivol -f -w ip2xboot ${DESTDIR}/usr/mdec/ip2xboot \
		${SGI.image}; \
	${TOOLDIR}/bin/nbsgivol -f -w ip3xboot ${DESTDIR}/usr/mdec/ip3xboot \
		${SGI.image}; \
	dd if=${CDROM.image} of=${SGI.image} bs=512 seek=$${SEEK}
	mv ${SGI.image} ${CDROM.image}
	rm -f ${RELEASEDIR}/ip2x ${RELEASEDIR}/ip3x

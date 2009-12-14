#	$NetBSD: Makefile.inc,v 1.19 2009/12/14 01:12:05 matt Exp $
#
#	etc.sgimips/Makefile.inc -- sgimips-specific etc Makefile targets
#

# If you change the list of distributed kernels, don't forget
# to update the release documentation in distrib/notes/common/contents

.if ${MACHINE_ARCH} == "mipseb"
SFX=32
.else
SFX=64
.endif

KERNEL_SETS=		GENERIC${SFX}_IP2x GENERIC${SFX}_IP3x
.if ${MACHINE_ARCH} == "mipseb"
KERNEL_SETS=		GENERIC32_IP12
KERNEL_SUFFIXES=	ecoff
.endif

#EXTRA_KERNELS=		NETBOOT

BUILD_KERNELS=		INSTALL${SFX}_IP2x INSTALL${SFX}_IP3x

INSTALLATION_DIRS+=	installation/netboot \
			installation/diskimage

iso-image-md-pre:
	cp ${RELEASEDIR}/${RELEASEMACHINEDIR}/binary/kernel/netbsd-INSTALL${SFX}_IP2x.gz ${RELEASEDIR}/ip2x
	cp ${RELEASEDIR}/${RELEASEMACHINEDIR}/binary/kernel/netbsd-INSTALL${SFX}_IP3x.gz ${RELEASEDIR}/ip3x
	echo "ip2x/=${RELEASEDIR}/ip2x/" >> ${CDROM.pathlist}
	echo "ip3x/=${RELEASEDIR}/ip3x/" >> ${CDROM.pathlist}

SGI.image= ${RELEASEDIR}/${RELEASEMACHINEDIR}/installation/cdrom/netbsd-sgimips.img

iso-image-md-post:
	@SEEK=`${TOOL_AWK} '/SGI_BOOT_BLOCK_SIZE_VOLHDR/{print $$3}' \
		${DESTDIR}/usr/include/sys/bootblock.h` ; \
	EST=`ls -l ${CDROM.image} \
		| ${TOOL_AWK} '{ s += $$5 } \
			END { print int(('$${SEEK}'*512+s)/1024/1024/32+1)*32 \
			}'` ; \
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

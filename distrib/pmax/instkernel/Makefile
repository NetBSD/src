#	$NetBSD: Makefile,v 1.22 2002/03/12 06:26:34 lukem Exp $

.include "${.CURDIR}/../../Makefile.inc"
.include <bsd.own.mk>
.include <bsd.kernobj.mk>

DISKBINDIR=	/installation/diskimage
KERNBINDIR=	/binary/kernel

KERN=		${KERNOBJDIR}/RAMDISK/netbsd
RAMDISKDIR!=	cd ${.CURDIR}/../ramdisk && ${PRINTOBJDIR}
RAMDISK=	${RAMDISKDIR}/ramdisk.fs

IMAGE=		diskimage
IMAGESIZE=	4096b

LISTS=		${.CURDIR}/list
IMAGEENDIAN=	le
IMAGEDEPENDS=	netbsd.gz ${DESTDIR}/usr/mdec/bootxx_ffs
# MAKEFS_FLAGS=	-o density=32k			# XXXDISTRIB

CLEANFILES+=	netbsd netbsd.gz netbsd.ecoff netbsd.ecoff.gz \
		${IMAGE} ${IMAGE}.gz


netbsd.gz: ${KERN} ${RAMDISK}
	cp ${KERN} netbsd
	${MDSETIMAGE} -v netbsd ${RAMDISK}
	elf2ecoff netbsd netbsd.ecoff		# XXXDISTRIB
	rm -f netbsd.ecoff.gz
	gzip -9 netbsd.ecoff
	rm -f netbsd.gz
	gzip -9 netbsd

${IMAGE}.gz:	${IMAGE}
	${DESTDIR}/usr/mdec/installboot ${IMAGE} ${DESTDIR}/usr/mdec/bootxx_ffs
						# XXXDISTRIB
	rm -f ${IMAGE}.gz
	gzip -9 ${IMAGE}


realall: netbsd.gz ${IMAGE}.gz

release: check_RELEASEDIR .WAIT netbsd.gz ${IMAGE}.gz
	${INSTALL} ${RENAME} ${PRESERVE} ${COPY} -m ${NONBINMODE} \
		${IMAGE}.gz ${RELEASEDIR}${DISKBINDIR}
	${INSTALL} ${RENAME} ${PRESERVE} ${COPY} -m ${NONBINMODE} \
		netbsd.gz ${RELEASEDIR}${KERNBINDIR}/install.gz
	${INSTALL} ${RENAME} ${PRESERVE} ${COPY} -m ${NONBINMODE} \
		netbsd.ecoff.gz ${RELEASEDIR}${KERNBINDIR}/install.ecoff.gz


.include "${DISTRIBDIR}/common/Makefile.image"

.include <bsd.prog.mk>

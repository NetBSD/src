#	$NetBSD: Makefile,v 1.12 2003/07/10 10:34:16 lukem Exp $

.include <bsd.own.mk>
.include "${NETBSDSRCDIR}/distrib/common/Makefile.distrib"

.include <bsd.kernobj.mk>

INSTKERNOBJ!=	cd ${.CURDIR}/../ramdisk-kernel && ${PRINTOBJDIR}

IMAGE=		boot.fs
IMAGESIZE=	1200k
XXBOOT=		${DESTDIR}/usr/mdec/xxboot_ustarfs 

IMAGEDEPENDS=	${XXBOOT} boot.vax netbsd.gz
CLEANFILES+=	netbsd.gz boot.vax ${IMAGE}.tmp

realall: ${IMAGE} ${XXBOOT} ${DESTDIR}/usr/mdec/boot

netbsd.gz: ${INSTKERNOBJ}/install.ram.gz
	cp -f ${.ALLSRC} ${.TARGET}

boot.vax: ${DESTDIR}/usr/mdec/boot
	cp -f ${.ALLSRC} ${.TARGET}

${IMAGE}: ${IMAGEDEPENDS}
	cat ${XXBOOT} > ${IMAGE}.tmp
	${TOOL_PAX} -O -w -x ustar boot.vax netbsd.gz | dd of=${IMAGE}.tmp seek=16
	dd if=${IMAGE}.tmp of=${IMAGE} bs=${IMAGESIZE} conv=sync

release: check_RELEASEDIR .WAIT ${IMAGE}
	${RELEASE_INSTALL} ${IMAGE} \
	    ${RELEASEDIR}/${MACHINE}/installation/bootfs

.include <bsd.prog.mk>

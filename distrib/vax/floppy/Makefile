#	$NetBSD: Makefile,v 1.13.48.1 2017/04/21 16:51:16 bouyer Exp $

.include <bsd.own.mk>
.include "${NETBSDSRCDIR}/distrib/common/Makefile.distrib"

.include <bsd.kernobj.mk>

.if ${MKREPRO_TIMESTAMP:Uno} != "no"
MAKEFS_TIMESTAMP=-T "${MKREPRO_TIMESTAMP}"
PAX_TIMESTAMP=--timestamp "${MKREPRO_TIMESTAMP}"
.endif

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
	${TOOL_PAX} ${PAX_TIMESTAMP} -O -w -x ustar boot.vax netbsd.gz | dd of=${IMAGE}.tmp seek=16
	dd if=${IMAGE}.tmp of=${IMAGE} bs=${IMAGESIZE} conv=sync

release: check_RELEASEDIR .WAIT ${IMAGE}
	${RELEASE_INSTALL} ${IMAGE} \
	    ${RELEASEDIR}/${RELEASEMACHINEDIR}/installation/bootfs

.include <bsd.prog.mk>

#	$NetBSD: Makefile,v 1.4 2002/03/29 22:53:30 matt Exp $

.include "${.CURDIR}/../../Makefile.inc"
.include <bsd.own.mk>
.include <bsd.kernobj.mk>

INSTKERNOBJ!=	cd ${.CURDIR}/../ramdisk-kernel && ${PRINTOBJDIR}

IMAGE=		boot.fs
IMAGESIZE=	1200k
XXBOOT=		${DESTDIR}/usr/mdec/xxboot_ustarfs 

IMAGEDEPENDS=	${XXBOOT} boot netbsd.gz
CLEANFILES+=	netbsd.gz boot ${IMAGE}.tmp

realall: ${IMAGE} ${XXBOOT} ${DESTDIR}/usr/mdec/boot

netbsd.gz: ${INSTKERNOBJ}/netbsd.INSTALL.gz 
	cp -f ${.ALLSRC} ${.TARGET}

boot: ${DESTDIR}/usr/mdec/boot
	cp -f ${.ALLSRC} ${.TARGET}

${IMAGE}: ${IMAGEDEPENDS}
	echo USTARFS 0 > ${IMAGE}.tmp
	pax -w -x ustar boot netbsd.gz | dd of=${IMAGE}.tmp seek=16
.if ${USETOOLS} == "yes"
	${TOOLDIR}/bin/nbvax-installboot -v ${IMAGE}.tmp ${XXBOOT}
.else
	${DESTDIR}/usr/mdec/installboot -v ${IMAGE}.tmp ${XXBOOT}
.endif
	dd if=${IMAGE}.tmp of=${IMAGE} obs=${IMAGESIZE} conv=osync

.include <bsd.prog.mk>

#	$NetBSD: Makefile,v 1.2 2002/03/28 04:51:25 lukem Exp $

.include "${.CURDIR}/../../Makefile.inc"
.include <bsd.own.mk>
.include <bsd.kernobj.mk>

INSTKERNOBJ!=	cd ${.CURDIR}/../ramdisk-kernel && ${PRINTOBJDIR}

IMAGE=		boot.fs
IMAGESIZE=	1440k

LISTS=		${.CURDIR}/list
IMAGEENDIAN=	le
IMAGEDEPENDS=	${DESTDIR}/usr/mdec/boot ${DESTDIR}/usr/mdec/xxboot \
		${INSTKERNOBJ}/netbsd.INSTALL.gz 

PARSELISTENV+=	INSTKERNOBJ=${INSTKERNOBJ:Q}

realall: ${IMAGE} ${DESTDIR}/usr/mdec/xxboot
	${DESTDIR}/usr/mdec/installboot -v ${IMAGE} ${DESTDIR}/usr/mdec/xxboot
		# XXXDISTRIB

.include "${DISTRIBDIR}/common/Makefile.image"

.include <bsd.prog.mk>

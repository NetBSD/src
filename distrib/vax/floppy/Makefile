#	$NetBSD: Makefile,v 1.3 2002/03/29 06:36:04 matt Exp $

.include "${.CURDIR}/../../Makefile.inc"
.include <bsd.own.mk>
.include <bsd.kernobj.mk>

INSTKERNOBJ!=	cd ${.CURDIR}/../ramdisk-kernel && ${PRINTOBJDIR}

IMAGE=		boot.fs

IMAGEDEPENDS=	${DESTDIR}/usr/mdec/boot ${DESTDIR}/usr/mdec/xxboot_ustarfs \
		netbsd.gz
CLEANFILES+=	netbsd.gz ${IMAGE}.tar

realall: ${IMAGE} ${DESTDIR}/usr/mdec/xxboot_ustarfs ${DESTDIR}/usr/mdec/boot

netbsd.gz: ${INSTKERNOBJ}/netbsd.INSTALL.gz 
	cp -f ${.ALLSRC} ${.TARGET}

${IMAGE}: ${IMAGEDEPENDS}
	( dd if=${DESTDIR}/usr/mdec/xxboot_ustarfs bs=8k conv=sync && \
	  tar -cf - -b 2384 \
		-C ${DESTDIR}/usr/mdec boot \
		-C ${.OBJDIR} netbsd.gz) > ${IMAGE}.tmp
	mv ${IMAGE}.tmp ${IMAGE}

.include <bsd.prog.mk>

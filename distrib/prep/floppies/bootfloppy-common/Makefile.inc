#	$NetBSD: Makefile.inc,v 1.8 2003/01/03 15:34:34 lukem Exp $

.include <bsd.own.mk>
.include "${NETBSDSRCDIR}/distrib/common/Makefile.distrib"

# Required variables:
#	IMAGE
#	KERNEL

MDEC?=		${DESTDIR}/usr/mdec
MKBOOTIMAGE?=	${PREPMKBOOTIMAGE}
BOOTLOADER?=	${MDEC}/boot_com0

${IMAGE}: ${KERNEL}
	${MKBOOTIMAGE} ${BOOTLOADER} ${.TARGET} ${KERNEL}	# XXXDISTRIB

${IMAGE}.gz: ${IMAGE}
	gzip -9c ${IMAGE} > ${.TARGET}

KFILES=	${IMAGE} ${IMAGE}.gz

CLEANFILES+=	${KFILES}


realall: ${KFILES}

release: check_RELEASEDIR .WAIT ${KFILES}
	${RELEASE_INSTALL} ${KFILES} \
	    ${RELEASEDIR}/${MACHINE}/installation/floppy

.include <bsd.prog.mk>

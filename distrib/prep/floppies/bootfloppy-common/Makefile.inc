#	$NetBSD: Makefile.inc,v 1.7 2002/05/03 20:54:18 kleink Exp $

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
	${RELEASE_INSTALL} ${KFILES} ${RELEASEDIR}/installation/floppy

.include <bsd.prog.mk>

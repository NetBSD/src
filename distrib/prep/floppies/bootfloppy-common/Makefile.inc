#	$NetBSD: Makefile.inc,v 1.4 2002/04/12 14:52:57 lukem Exp $

.include <bsd.own.mk>
.include "${_SRC_TOP_}/distrib/Makefile.inc"

# Required variables:
#	IMAGE
#	KERNEL

MDEC?=		${DESTDIR}/usr/mdec
MKBOOTIMAGE?=	${MDEC}/mkbootimage
BOOTLOADER?=	${MDEC}/boot_com0

${IMAGE}: ${KERNEL}
	${MKBOOTIMAGE} ${BOOTLOADER} ${.TARGET} ${KERNEL}	# XXXDISTRIB

${IMAGE}.gz: ${IMAGE}
	gzip -9c ${IMAGE} > ${.TARGET}

KFILES=	${IMAGE} ${IMAGE}.gz

CLEANFILES+=	${KFILES}


realall: ${KFILES}

release: check_RELEASEDIR .WAIT ${KFILES}
	-mkdir -p ${RELEASEDIR}/installation/floppy
	${RELINSTALL} ${KFILES} ${RELEASEDIR}/installation/floppy

.include <bsd.prog.mk>

#	$NetBSD: Makefile.inc,v 1.9 2003/07/10 10:34:13 lukem Exp $

.include <bsd.own.mk>
.include "${NETBSDSRCDIR}/distrib/common/Makefile.distrib"

# Required variables:
#	IMAGE
#	KERNEL

MDEC?=		${DESTDIR}/usr/mdec
BOOTLOADER?=	${MDEC}/boot_com0

${IMAGE}: ${KERNEL}
	${TOOL_PREPMKBOOTIMAGE} ${BOOTLOADER} ${.TARGET} ${KERNEL} # XXXDISTRIB

${IMAGE}.gz: ${IMAGE}
	gzip -9c ${IMAGE} > ${.TARGET}

KFILES=	${IMAGE} ${IMAGE}.gz

CLEANFILES+=	${KFILES}


realall: ${KFILES}

release: check_RELEASEDIR .WAIT ${KFILES}
	${RELEASE_INSTALL} ${KFILES} \
	    ${RELEASEDIR}/${MACHINE}/installation/floppy

.include <bsd.prog.mk>

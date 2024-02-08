#	$NetBSD: Makefile.inc,v 1.14 2024/02/08 17:57:54 christos Exp $

.include <bsd.own.mk>
.include "${NETBSDSRCDIR}/distrib/common/Makefile.distrib"

# Required variables:
#	IMAGE

MDEC?=		${DESTDIR}/usr/mdec
BOOTLOADER?=	${MDEC}/boot

.if ${MKREPRO_TIMESTAMP:Uno} != "no"
MKBOOTIMAGE_TIMESTAMP=-t "${MKREPRO_TIMESTAMP}"
.endif


${IMAGE}:
	${TOOL_POWERPCMKBOOTIMAGE} -I -m ${MACHINE} -b ${BOOTLOADER} ${MKBOOTIMAGE_TIMESTAMP} ${.TARGET}

KFILES=	${IMAGE}

CLEANFILES+=	${KFILES}


realall: ${KFILES}

release: check_RELEASEDIR .WAIT ${KFILES}
	${RELEASE_INSTALL} ${KFILES} \
	    ${RELEASEDIR}/${RELEASEMACHINEDIR}/installation/floppy

.include <bsd.prog.mk>

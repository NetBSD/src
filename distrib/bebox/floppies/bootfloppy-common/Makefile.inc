#	$NetBSD: Makefile.inc,v 1.13 2010/10/17 11:19:53 kiyohara Exp $

.include <bsd.own.mk>
.include "${NETBSDSRCDIR}/distrib/common/Makefile.distrib"

# Required variables:
#	IMAGE

MDEC?=		${DESTDIR}/usr/mdec
BOOTLOADER?=	${MDEC}/boot

${IMAGE}:
	${TOOL_POWERPCMKBOOTIMAGE} -I -m ${MACHINE} -b ${BOOTLOADER} ${.TARGET}

KFILES=	${IMAGE}

CLEANFILES+=	${KFILES}


realall: ${KFILES}

release: check_RELEASEDIR .WAIT ${KFILES}
	${RELEASE_INSTALL} ${KFILES} \
	    ${RELEASEDIR}/${RELEASEMACHINEDIR}/installation/floppy

.include <bsd.prog.mk>

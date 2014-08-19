#	$NetBSD: Makefile.inc,v 1.12.24.1 2014/08/19 23:45:42 tls Exp $

.include <bsd.own.mk>
.include "${NETBSDSRCDIR}/distrib/common/Makefile.distrib"

# Required variables:
#	IMAGE
#	KERNEL

MDEC?=		${DESTDIR}/usr/mdec
BOOTLOADER?=	${MDEC}/boot_com0
TMPKERNEL=	${.OBJDIR}/netbsd

${IMAGE}: ${KERNEL}
	${TOOL_GZIP} -cd ${KERNEL} > ${TMPKERNEL}
	${TOOL_POWERPCMKBOOTIMAGE} -m ${MACHINE} -b ${BOOTLOADER} 	\
		-k ${TMPKERNEL} ${.TARGET}
	-rm -f ${TMPKERNEL}

KFILES=	${IMAGE}

CLEANFILES+=	${KFILES}


realall: ${KFILES}

release: check_RELEASEDIR .WAIT ${KFILES}
	${RELEASE_INSTALL} ${KFILES} \
	    ${RELEASEDIR}/${RELEASEMACHINEDIR}/installation/floppy

.include <bsd.prog.mk>

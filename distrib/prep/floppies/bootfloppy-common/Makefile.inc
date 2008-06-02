#	$NetBSD: Makefile.inc,v 1.10.20.1 2008/06/02 13:21:17 mjf Exp $

.include <bsd.own.mk>
.include "${NETBSDSRCDIR}/distrib/common/Makefile.distrib"

# Required variables:
#	IMAGE
#	KERNEL

MDEC?=		${DESTDIR}/usr/mdec
BOOTLOADER?=	${MDEC}/boot_com0
TMPKERNEL=	${.OBJDIR}/netbsd

${IMAGE}: ${KERNEL}
	gzip -cd ${KERNEL} > ${TMPKERNEL}
	${TOOL_POWERPCMKBOOTIMAGE} -m ${MACHINE} -b ${BOOTLOADER} 	\
		-k ${TMPKERNEL} ${.TARGET}
	-rm -f ${TMPKERNEL}

KFILES=	${IMAGE}

CLEANFILES+=	${KFILES}


realall: ${KFILES}

release: check_RELEASEDIR .WAIT ${KFILES}
	${RELEASE_INSTALL} ${KFILES} \
	    ${RELEASEDIR}/${MACHINE}/installation/floppy

.include <bsd.prog.mk>

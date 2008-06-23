#	$NetBSD: Makefile.inc,v 1.12 2008/06/23 02:16:54 matt Exp $

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
	    ${RELEASEDIR}/${RELEASEMACHINEDIR}/installation/floppy

.include <bsd.prog.mk>

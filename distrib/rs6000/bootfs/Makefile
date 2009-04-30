#	$NetBSD: Makefile,v 1.1 2009/04/30 12:47:39 tsutsui Exp $

.include <bsd.own.mk>
.include "${NETBSDSRCDIR}/distrib/common/Makefile.distrib"

IMAGE=		boot.fs

MDEC=		${DESTDIR}/usr/mdec
BOOTLOADER=	${MDEC}/boot

KERNDIR!=	cd ${.CURDIR}/../instkernel && ${PRINTOBJDIR}
KERNEL=		${KERNDIR}/netbsd-INSTALL.gz

TMPKERNEL=	${.OBJDIR}/netbsd

BOOT_RELEASEDIR=	installation/bootfs

${IMAGE}.gz:	${KERNEL} ${BOOTLOADER}
	gzip -cd ${KERNEL} > ${TMPKERNEL}
	${TOOL_POWERPCMKBOOTIMAGE} -m ${MACHINE} \
	    -b ${BOOTLOADER} -k ${TMPKERNEL} ${IMAGE}
	gzip -9nc ${IMAGE} > ${IMAGE}.tmp
	mv ${IMAGE}.tmp ${.TARGET}

CLEANFILES+=	${IMAGE} ${IMAGE}.tmp ${IMAGE}.gz

all realall:	${IMAGE}.gz

release::	check_RELEASEDIR .WAIT ${IMAGE}.gz
	${RELEASE_INSTALL} ${IMAGE}.gz \
	    ${RELEASEDIR}/${RELEASEMACHINEDIR}/${BOOT_RELEASEDIR}

.include <bsd.prog.mk>

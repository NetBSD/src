#	$NetBSD: Makefile,v 1.2 2001/12/07 06:57:49 minoura Exp $

TOP=		${.CURDIR}/..

.include "${TOP}/Makefile.inc"
.include <bsd.kernobj.mk>
.include <bsd.own.mk>

BASENAME=	kern
IMAGE1=		${BASENAME}.fs
INSTKERNEL=	${.OBJDIR}/netbsd.tmp

netbsd: ${INSTKERNEL}

${INSTKERNEL}:
	gzip -9vc ${KERNOBJDIR}/GENERIC/netbsd > netbsd.tmp

.include "${TOP}/bootfloppy-common/Makefile.inc"

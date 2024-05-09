#	$NetBSD: Makefile,v 1.5 2024/05/09 12:09:59 pho Exp $

.include "../Makefile.inc"

.if ${MACHINE_CPU} == "aarch64"
.PATH:	${S}/arch/aarch64/aarch64
.elif ${MACHINE_ARCH} == "i386" || \
    ${MACHINE_ARCH} == "x86_64"
.PATH:	${S}/arch/x86/x86
.endif
.PATH:	${S}/dev/vmt

KMOD=	vmt
IOCONF=	vmt.ioconf
SRCS=	vmt.c \
	vmt_subr.c

.include <bsd.kmodule.mk>

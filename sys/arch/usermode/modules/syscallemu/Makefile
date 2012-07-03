#	$NetBSD: Makefile,v 1.3 2012/07/03 21:07:41 christos Exp $

.include <bsd.own.mk>

S?=	${NETBSDSRCDIR}/sys
M?=	${S}/modules

.include "${M}/Makefile.inc"
.include "${M}/Makefile.assym"

KMOD=	syscallemu

SRCS=	syscallemu.c
.if ${MACHINE_ARCH} == "i386" || ${MACHINE_ARCH} == "x86_64"
SRCS+=	syscallemu_x86.c
.else
.error ${MACHINE_ARCH} not implemented
.endif

.include <bsd.kmodule.mk>

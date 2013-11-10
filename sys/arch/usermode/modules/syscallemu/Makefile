#	$NetBSD: Makefile,v 1.4 2013/11/10 19:58:38 jmcneill Exp $

.include <bsd.own.mk>

S?=	${NETBSDSRCDIR}/sys
M?=	${S}/modules

.include "${M}/Makefile.inc"
.include "${M}/Makefile.assym"

KMOD=	syscallemu

SRCS=	syscallemu.c
.if ${MACHINE_ARCH} == "i386" || ${MACHINE_ARCH} == "x86_64"
SRCS+=	syscallemu_x86.c
.elif ${MACHINE_ARCH} == "arm" || ${MACHINE_ARCH} == "earm"
SRCS+=	syscallemu_arm.c
.else
.error ${MACHINE_ARCH} not implemented
.endif

.include <bsd.kmodule.mk>

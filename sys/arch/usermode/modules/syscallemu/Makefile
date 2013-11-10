#	$NetBSD: Makefile,v 1.5 2013/11/10 22:08:58 jmcneill Exp $

.include <bsd.own.mk>

S?=	${NETBSDSRCDIR}/sys
M?=	${S}/modules

.include "${M}/Makefile.inc"
.include "${M}/Makefile.assym"

KMOD=	syscallemu

SRCS=	syscallemu.c
.if ${MACHINE_CPU} == "i386" || ${MACHINE_CPU} == "x86_64"
SRCS+=	syscallemu_x86.c
.elif ${MACHINE_CPU} == "arm"
SRCS+=	syscallemu_arm.c
.else
.error ${MACHINE_CPU} not implemented
.endif

.include <bsd.kmodule.mk>

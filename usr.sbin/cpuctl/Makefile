#	$NetBSD: Makefile,v 1.1.8.1 2008/05/18 12:36:14 yamt Exp $

.PATH:	${.CURDIR}/arch

PROG=	cpuctl
MAN=	cpuctl.8
SRCS=	cpuctl.c bitmask.c

.if exists(${.CURDIR}/arch/${MACHINE_ARCH}.c)
SRCS+=	${MACHINE_ARCH}.c
.if exists(${.CURDIR}/arch/${MACHINE_ARCH}-asm.S)
SRCS+=	${MACHINE_ARCH}-asm.S
.endif
.else
SRCS+=	noarch.c
.endif

.include <bsd.prog.mk>

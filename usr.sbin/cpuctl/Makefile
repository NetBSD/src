#	$NetBSD: Makefile,v 1.2 2008/05/05 17:54:14 ad Exp $

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

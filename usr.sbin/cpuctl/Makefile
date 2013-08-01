#	$NetBSD: Makefile,v 1.7 2013/08/01 23:19:39 matt Exp $

.include <bsd.own.mk>

.PATH:	${.CURDIR}/arch

PROG=	cpuctl
MAN=	cpuctl.8
SRCS=	cpuctl.c

.if exists(${.CURDIR}/arch/${MACHINE_ARCH}.c)
SRCS+=	${MACHINE_ARCH}.c
.elif exists(${.CURDIR}/arch/${MACHINE_CPU}.c)
SRCS+=	${MACHINE_CPU}.c
.else
SRCS+=	noarch.c
.endif
.if exists(${.CURDIR}/arch/${MACHINE_ARCH}-asm.S)
SRCS+=	${MACHINE_ARCH}-asm.S
.endif

LDADD+=-lutil
DPADD+=${LIBUTIL}

.include <bsd.prog.mk>

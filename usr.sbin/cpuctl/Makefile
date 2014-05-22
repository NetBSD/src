#	$NetBSD: Makefile,v 1.5.6.1 2014/05/22 11:43:02 yamt Exp $

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

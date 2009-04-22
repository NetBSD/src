#	$NetBSD: Makefile,v 1.4 2009/04/22 15:23:02 lukem Exp $

WARNS?=	3	# XXX -Wsign-compare issues in arch/i386.c

.PATH:	${.CURDIR}/arch

PROG=	cpuctl
MAN=	cpuctl.8
SRCS=	cpuctl.c

.if exists(${.CURDIR}/arch/${MACHINE_ARCH}.c)
SRCS+=	${MACHINE_ARCH}.c
.if exists(${.CURDIR}/arch/${MACHINE_ARCH}-asm.S)
SRCS+=	${MACHINE_ARCH}-asm.S
.endif
.else
SRCS+=	noarch.c
.endif

LDADD+=-lutil
DPADD+=${LIBUTIL}

.include <bsd.prog.mk>

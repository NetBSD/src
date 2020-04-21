#	$NetBSD: Makefile,v 1.8.16.1 2020/04/21 18:42:47 martin Exp $

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

.if ${MACHINE_ARCH} == "x86_64" || ${MACHINE_ARCH} == "i386"
CPPFLAGS+= -I${.CURDIR} -I${.CURDIR}/arch
.PATH.c: ${NETBSDSRCDIR}/sys/arch/x86/x86
SRCS+=	identcpu_subr.c
.endif

CPPFLAGS+=	-D_KERNTYPES
LDADD+=-lutil
DPADD+=${LIBUTIL}

.include <bsd.prog.mk>

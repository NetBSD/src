#	$NetBSD: Makefile,v 1.4 2018/07/13 07:56:29 maxv Exp $

.PATH:	${.CURDIR}/arch

PROG=	tprof
MAN=	tprof.8
SRCS=	tprof.c

.if	${MACHINE_ARCH} == "i386" || ${MACHINE_ARCH} == "x86_64"
SRCS+=	tprof_x86.c
.else
SRCS+=	tprof_noarch.c
.endif

CPPFLAGS+= -I${NETBSDSRCDIR}/sys/

LDADD+= -lpthread
DPADD+= ${LIBPTHREAD}

.include <bsd.own.mk>
.include <bsd.prog.mk>

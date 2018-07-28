#	$NetBSD: Makefile,v 1.3.38.1 2018/07/28 04:38:15 pgoyette Exp $

.PATH:	${.CURDIR}/arch

PROG=	tprof
MAN=	tprof.8
SRCS=	tprof.c tprof_analyze.c

.if	${MACHINE_ARCH} == "i386" || ${MACHINE_ARCH} == "x86_64"
SRCS+=	tprof_x86.c
.elif	!empty(MACHINE_ARCH:M*armv7*)
SRCS+=	tprof_armv7.c
.elif	${MACHINE_ARCH} == "aarch64"
SRCS+=	tprof_armv8.c
.else
SRCS+=	tprof_noarch.c
.endif

CPPFLAGS+= -I${NETBSDSRCDIR}/sys/

LDADD+= -lpthread
LDADD+= -lelf
LDADD+= -lutil
DPADD+= ${LIBPTHREAD}
DPADD+= ${LIBELF}
DPADD+= ${LIBUTIL}

.include <bsd.own.mk>
.include <bsd.prog.mk>

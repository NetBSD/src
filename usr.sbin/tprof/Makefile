#	$NetBSD: Makefile,v 1.12 2022/12/01 00:43:27 ryo Exp $

.PATH:	${.CURDIR}/arch

PROG=	tprof
MAN=	tprof.8
SRCS=	tprof.c tprof_analyze.c tprof_top.c ksyms.c

.if	${MACHINE_ARCH} == "i386" || ${MACHINE_ARCH} == "x86_64"
SRCS+=	tprof_x86.c
.elif	!empty(MACHINE_ARCH:M*armv7*)
SRCS+=	tprof_armv7.c
.elif	!empty(MACHINE_ARCH:Maarch64*)
SRCS+=	tprof_armv8.c
.else
SRCS+=	tprof_noarch.c
.endif

CPPFLAGS+= -I${NETBSDSRCDIR}/sys/

LDADD+= -lpthread
LDADD+= -lm
LDADD+= -lelf
LDADD+= -lutil
DPADD+= ${LIBPTHREAD}
DPADD+= ${LIBM}
DPADD+= ${LIBELF}
DPADD+= ${LIBUTIL}

.include <bsd.own.mk>
.include <bsd.prog.mk>

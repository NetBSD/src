#	$NetBSD: Makefile,v 1.16.28.1 2020/04/13 08:05:51 martin Exp $

.include <bsd.own.mk>

.if (${MACHINE_ARCH} == "i386" || \
     ${MACHINE_CPU} == "arm" || \
     ${MACHINE} == "hpcmips" || \
     ${MACHINE} == "hpcsh" || \
     ${MACHINE} == "macppc" || \
     ${MACHINE} == "sparc")
PROG=	apmd
SRCS=	apmd.c apmsubr.c
.endif

MAN=	apmd.8

LDADD+=	-lutil
DPADD+=	${LIBUTIL}

COPTS.apmd.c+=	${GCC_NO_STRINGOP_TRUNCATION}

.include <bsd.prog.mk>

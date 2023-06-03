#	$NetBSD: Makefile,v 1.18 2023/06/03 09:09:20 lukem Exp $

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

COPTS.apmd.c+=	${CC_WNO_STRINGOP_TRUNCATION}

.include <bsd.prog.mk>

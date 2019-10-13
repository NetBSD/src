#	$NetBSD: Makefile,v 1.17 2019/10/13 07:28:18 mrg Exp $

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

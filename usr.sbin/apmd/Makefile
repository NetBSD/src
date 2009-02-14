#	$NetBSD: Makefile,v 1.13 2009/02/14 13:56:41 abs Exp $

.if (${MACHINE_ARCH} == "i386" || \
     ${MACHINE} == "hpcmips" || \
     ${MACHINE} == "hpcsh" || \
     ${MACHINE} == "macppc" || \
     ${MACHINE_ARCH} == "sparc" || \
     ${MACHINE} == "zaurus")
PROG=	apmd
SRCS=	apmd.c apmsubr.c
.endif

MAN=	apmd.8

WARNS=  3

LDADD+=	-lutil
DPADD+=	${LIBUTIL}

.include <bsd.prog.mk>

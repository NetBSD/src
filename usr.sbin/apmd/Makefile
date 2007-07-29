#	$NetBSD: Makefile,v 1.12.16.2 2007/07/29 14:34:46 nonaka Exp $

.if (${MACHINE} == "i386" || \
     ${MACHINE} == "hpcmips" || \
     ${MACHINE} == "hpcsh" || \
     ${MACHINE} == "macppc" || \
     ${MACHINE} == "sparc" || \
     ${MACHINE} == "zaurus")
PROG=	apmd
SRCS=	apmd.c apmsubr.c
.endif

MAN=	apmd.8

WARNS=  3

LDADD+=	-lutil
DPADD+=	${LIBUTIL}

.include <bsd.prog.mk>

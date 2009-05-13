#	$NetBSD: Makefile,v 1.12.20.1 2009/05/13 19:20:15 jym Exp $

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

LDADD+=	-lutil
DPADD+=	${LIBUTIL}

.include <bsd.prog.mk>

#	$NetBSD: Makefile,v 1.14 2009/04/22 15:23:02 lukem Exp $

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

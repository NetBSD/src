#	$NetBSD: Makefile,v 1.7 1999/12/15 20:34:03 garbled Exp $

.if ${MACHINE} == "i386" || ${MACHINE} == "sparc"
PROG=	apmd
SRCS=	apmd.c apmsubr.c
.endif

MAN=	apmd.8

LDADD+=	-lutil
DPADD+=	${LIBUTIL}

.include <bsd.prog.mk>

#	$NetBSD: Makefile,v 1.6 1999/06/06 03:17:23 thorpej Exp $

.if ${MACHINE} == "i386"
PROG=	apmd
SRCS=	apmd.c apmsubr.c
.endif

MAN=	apmd.8
MANSUBDIR=/i386

LDADD+=	-lutil
DPADD+=	${LIBUTIL}

.include <bsd.prog.mk>

#	$NetBSD: Makefile,v 1.8 1999/06/06 03:21:43 thorpej Exp $

PROG=	mopd
SRCS=	mopd.c process.c
MAN=	mopd.8

LDADD+=	-lutil
DPADD+=	${LIBUTIL}

.include <bsd.prog.mk>

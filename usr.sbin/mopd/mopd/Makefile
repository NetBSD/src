#	$NetBSD: Makefile,v 1.12 2012/08/10 12:10:29 joerg Exp $

PROG=	mopd
SRCS=	mopd.c process.c
MAN=	mopd.8

LDADD+=	-lutil
DPADD+=	${LIBUTIL}

.include <bsd.prog.mk>

COPTS.process.c+=	-Wno-pointer-sign

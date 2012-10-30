#	$NetBSD: Makefile,v 1.11.2.1 2012/10/30 19:00:39 yamt Exp $

PROG=	mopd
SRCS=	mopd.c process.c
MAN=	mopd.8

LDADD+=	-lutil
DPADD+=	${LIBUTIL}

.include <bsd.prog.mk>

COPTS.process.c+=	-Wno-pointer-sign

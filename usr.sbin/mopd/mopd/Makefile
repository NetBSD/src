#	$NetBSD: Makefile,v 1.14 2021/10/31 18:06:42 christos Exp $

.include <bsd.init.mk>

PROG=	mopd
SRCS=	mopd.c process.c
MAN=	mopd.8

LDADD+=	-lutil
DPADD+=	${LIBUTIL}

COPTS.process.c+=	-Wno-pointer-sign ${GCC_NO_FORMAT_TRUNCATION}

.include <bsd.prog.mk>


#	$NetBSD: Makefile,v 1.15 2023/06/03 09:09:21 lukem Exp $

.include <bsd.init.mk>

PROG=	mopd
SRCS=	mopd.c process.c
MAN=	mopd.8

LDADD+=	-lutil
DPADD+=	${LIBUTIL}

COPTS.process.c+=	-Wno-pointer-sign ${CC_WNO_FORMAT_TRUNCATION}

.include <bsd.prog.mk>


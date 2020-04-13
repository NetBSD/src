#	$NetBSD: Makefile,v 1.12.32.1 2020/04/13 08:05:54 martin Exp $

PROG=	mopd
SRCS=	mopd.c process.c
MAN=	mopd.8

LDADD+=	-lutil
DPADD+=	${LIBUTIL}

.include <bsd.prog.mk>

COPTS.process.c+=	-Wno-pointer-sign ${GCC_NO_FORMAT_TRUNCATION}

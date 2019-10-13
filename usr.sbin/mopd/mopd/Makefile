#	$NetBSD: Makefile,v 1.13 2019/10/13 07:28:19 mrg Exp $

PROG=	mopd
SRCS=	mopd.c process.c
MAN=	mopd.8

LDADD+=	-lutil
DPADD+=	${LIBUTIL}

.include <bsd.prog.mk>

COPTS.process.c+=	-Wno-pointer-sign ${GCC_NO_FORMAT_TRUNCATION}

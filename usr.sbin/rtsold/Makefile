# $NetBSD: Makefile,v 1.15 2014/03/18 00:16:49 christos Exp $

WARNS?=6
USE_FORT?= yes	# network client (local server)

PROG=	rtsold
SRCS=	rtsold.c rtsol.c if.c probe.c dump.c rtsock.c

CPPFLAGS+=-DUSE_RTSOCK

MAN=	rtsold.8
MLINKS=	rtsold.8 rtsol.8

LDADD+=	-lutil
DPADD+=	${LIBUTIL}

.include <bsd.prog.mk>

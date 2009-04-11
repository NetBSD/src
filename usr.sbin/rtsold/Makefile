# $NetBSD: Makefile,v 1.13 2009/04/11 07:51:59 lukem Exp $

USE_FORT?= yes	# network client (local server)
WARNS?=	4

PROG=	rtsold
SRCS=	rtsold.c rtsol.c if.c probe.c dump.c rtsock.c

CPPFLAGS+=-DUSE_RTSOCK

MAN=	rtsold.8
MLINKS=	rtsold.8 rtsol.8

LDADD+=	-lutil
DPADD+=	${LIBUTIL}

.include <bsd.prog.mk>

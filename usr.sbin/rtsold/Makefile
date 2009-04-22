# $NetBSD: Makefile,v 1.14 2009/04/22 15:23:08 lukem Exp $

USE_FORT?= yes	# network client (local server)

PROG=	rtsold
SRCS=	rtsold.c rtsol.c if.c probe.c dump.c rtsock.c

CPPFLAGS+=-DUSE_RTSOCK

MAN=	rtsold.8
MLINKS=	rtsold.8 rtsol.8

LDADD+=	-lutil
DPADD+=	${LIBUTIL}

.include <bsd.prog.mk>

# $NetBSD: Makefile,v 1.8 2002/05/31 22:10:18 itojun Exp $

PROG=	rtsold
SRCS=	rtsold.c rtsol.c if.c probe.c dump.c rtsock.c

CPPFLAGS+=-DINET6 -DHAVE_GETIFADDRS -DHAVE_ARC4RANDOM -DUSE_RTSOCK

MAN=	rtsold.8
MLINKS=	rtsold.8 rtsol.8

LDADD+=	-lutil
DPADD+=	${LIBUTIL}

.include <bsd.prog.mk>

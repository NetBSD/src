# $NetBSD: Makefile,v 1.11 2004/01/03 01:40:31 itojun Exp $

PROG=	rtsold
SRCS=	rtsold.c rtsol.c if.c probe.c dump.c rtsock.c

CPPFLAGS+=-DUSE_RTSOCK

MAN=	rtsold.8
MLINKS=	rtsold.8 rtsol.8

LDADD+=	-lutil
DPADD+=	${LIBUTIL}

.include <bsd.prog.mk>

# $NetBSD: Makefile,v 1.10 2003/09/06 12:56:43 itojun Exp $

PROG=	rtsold
SRCS=	rtsold.c rtsol.c if.c probe.c dump.c rtsock.c

CPPFLAGS+=-DINET6 -DUSE_RTSOCK

MAN=	rtsold.8
MLINKS=	rtsold.8 rtsol.8

LDADD+=	-lutil
DPADD+=	${LIBUTIL}

.include <bsd.prog.mk>

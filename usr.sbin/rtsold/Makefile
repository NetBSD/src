# $NetBSD: Makefile,v 1.2 1999/09/03 05:25:43 itojun Exp $

PROG=	rtsold
SRCS=	rtsold.c rtsol.c if.c probe.c

CPPFLAGS+=-DINET6

LDADD+=	-lkvm
DPADD+=	${LIBKVM}

MAN=	rtsold.8
MLINKS=	rtsold.8 rtsol.8

.include <bsd.prog.mk>

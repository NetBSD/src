# $NetBSD: Makefile,v 1.1 1999/09/03 05:14:37 itojun Exp $

PROG=	rtsold
SRCS=	rtsold.c rtsol.c if.c probe.c

CPPFLAGS+=-DINET6

LDADD+=	-lkvm
DPADD+=	${LIBKVM}

MAN=	rtsold.8

.include <bsd.prog.mk>

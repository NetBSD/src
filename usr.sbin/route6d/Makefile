# $NetBSD: Makefile,v 1.6 2002/05/29 23:11:13 itojun Exp $

PROG=	route6d
MAN=	route6d.8

CPPFLAGS+=-DINET6

LDADD+=	-lutil
DPADD+=	${LIBUTIL}

.include <bsd.prog.mk>

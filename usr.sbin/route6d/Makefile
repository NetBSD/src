# $NetBSD: Makefile,v 1.7 2007/05/28 12:06:39 tls Exp $

USE_FORT?= yes	# network server

PROG=	route6d
MAN=	route6d.8

CPPFLAGS+=-DINET6

LDADD+=	-lutil
DPADD+=	${LIBUTIL}

.include <bsd.prog.mk>

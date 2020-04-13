# $NetBSD: Makefile,v 1.7.76.1 2020/04/13 08:05:57 martin Exp $

USE_FORT?= yes	# network server

PROG=	route6d
MAN=	route6d.8

CPPFLAGS+=-DINET6

LDADD+=	-lutil
DPADD+=	${LIBUTIL}

COPTS.route6d.c+=	${GCC_NO_STRINGOP_TRUNCATION}

.include <bsd.prog.mk>

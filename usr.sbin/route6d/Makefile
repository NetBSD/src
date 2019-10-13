# $NetBSD: Makefile,v 1.8 2019/10/13 07:28:20 mrg Exp $

USE_FORT?= yes	# network server

PROG=	route6d
MAN=	route6d.8

CPPFLAGS+=-DINET6

LDADD+=	-lutil
DPADD+=	${LIBUTIL}

COPTS.route6d.c+=	${GCC_NO_STRINGOP_TRUNCATION}

.include <bsd.prog.mk>

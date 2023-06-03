# $NetBSD: Makefile,v 1.9 2023/06/03 09:09:22 lukem Exp $

USE_FORT?= yes	# network server

PROG=	route6d
MAN=	route6d.8

CPPFLAGS+=-DINET6

LDADD+=	-lutil
DPADD+=	${LIBUTIL}

COPTS.route6d.c+=	${CC_WNO_STRINGOP_TRUNCATION}

.include <bsd.prog.mk>

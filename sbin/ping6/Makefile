# $NetBSD: Makefile,v 1.14 2011/08/14 12:09:35 christos Exp $

USE_FORT?= yes	# setuid
PROG=	ping6
MAN=	ping6.8

BINOWN=	root
BINMODE=4555

CPPFLAGS+=-DINET6
CPPFLAGS+=-DIPSEC

LDADD+=	-lipsec -lm
DPADD+=	${LIBIPSEC} ${LIBM}

.include <bsd.prog.mk>

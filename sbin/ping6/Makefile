# $NetBSD: Makefile,v 1.9 2005/06/27 01:00:06 christos Exp $

PROG=	ping6
MAN=	ping6.8

BINOWN=	root
BINMODE=4555

CPPFLAGS+=-DINET6
CPPFLAGS+=-DIPSEC

LDADD+=	-lipsec -lm
DPADD+=	${LIBIPSEC} ${LIBM}

.include <bsd.prog.mk>

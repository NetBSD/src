# $NetBSD: Makefile,v 1.7 2002/05/26 13:01:21 itojun Exp $

PROG=	ping6
MAN=	ping6.8

BINOWN=	root
BINMODE=4555

CPPFLAGS+=-DINET6
CPPFLAGS+=-DIPSEC

LDADD+=	-lipsec -lm
DPADD+=	${LIBIPSEC} ${LIBM}

.include <bsd.prog.mk>

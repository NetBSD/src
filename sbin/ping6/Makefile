# $NetBSD: Makefile,v 1.8 2005/02/09 14:09:46 xtraeme Exp $

PROG=	ping6
MAN=	ping6.8

WARNS=	3

BINOWN=	root
BINMODE=4555

CPPFLAGS+=-DINET6
CPPFLAGS+=-DIPSEC

LDADD+=	-lipsec -lm
DPADD+=	${LIBIPSEC} ${LIBM}

.include <bsd.prog.mk>

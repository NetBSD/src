# $NetBSD: Makefile,v 1.5.4.1 2000/10/18 17:04:38 tv Exp $

PROG=	ping6
MAN=	ping6.8

BINOWN=	root
BINMODE=4555

CPPFLAGS+=-DINET6
CPPFLAGS+=-DIPSEC

LDADD+=	-lipsec -lm
DPADD+=	${LIBIPSEC} ${LIBM}

# KAME scope id hack
CPPFLAGS+=-DKAME_SCOPEID

.include <bsd.prog.mk>

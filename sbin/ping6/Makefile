# $NetBSD: Makefile,v 1.4 1999/07/12 19:02:04 itojun Exp $

PROG=	ping6
MAN=	ping6.8

BINOWN=	root
BINMODE=4555

CPPFLAGS+=-DINET6
CPPFLAGS+=-DIPSEC

LDADD+=	-lipsec
DPADD+=	${LIBIPSEC}

.include <bsd.prog.mk>

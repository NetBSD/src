# $NetBSD: Makefile,v 1.5 1999/12/13 15:25:59 itojun Exp $

PROG=	ping6
MAN=	ping6.8

BINOWN=	root
BINMODE=4555

CPPFLAGS+=-DINET6
CPPFLAGS+=-DIPSEC

LDADD+=	-lipsec
DPADD+=	${LIBIPSEC}

# KAME scope id hack
CPPFLAGS+=-DKAME_SCOPEID

.include <bsd.prog.mk>

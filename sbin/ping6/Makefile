# $NetBSD: Makefile,v 1.4.4.1 1999/12/27 18:30:29 wrstuden Exp $

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

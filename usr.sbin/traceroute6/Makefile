# $NetBSD: Makefile,v 1.6 2006/05/05 00:03:22 rpaulo Exp $

PROG=	traceroute6
MAN=	traceroute6.8

BINMODE=4555
BINOWN=	root

CPPFLAGS+=-DINET6 -DIPSEC
CPPFLAGS+=-DUSE_RFC3542
LDADD+=	-lipsec
DPADD+=	${LIBIPSEC}

CPPFLAGS+=-DHAVE_POLL

.include <bsd.prog.mk>

# $NetBSD: Makefile,v 1.5 2000/10/08 06:40:42 itojun Exp $

PROG=	traceroute6
MAN=	traceroute6.8

BINMODE=4555
BINOWN=	root

CPPFLAGS+=-DINET6 -DIPSEC
LDADD+=	-lipsec
DPADD+=	${LIBIPSEC}

CPPFLAGS+=-DHAVE_POLL

.include <bsd.prog.mk>

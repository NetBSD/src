# $NetBSD: Makefile,v 1.7.20.1 2009/05/13 19:20:43 jym Exp $

WARNS?=	1	# XXX -Wshadow -Wcast-qual -Wsign-compare

USE_FORT?= yes	# network client

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

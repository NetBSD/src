# $NetBSD: Makefile,v 1.8 2009/04/22 15:23:09 lukem Exp $

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

# $NetBSD: Makefile,v 1.7 2007/05/28 12:06:42 tls Exp $

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

# $NetBSD: Makefile,v 1.3 2000/02/25 08:52:04 mycroft Exp $

PROG=	traceroute6
MAN=	traceroute6.8

BINMODE=4555
BINOWN=	root

CPPFLAGS+=-DINET6 -DIPSEC
LDADD+=	-lipsec
DPADD+=	${LIBIPSEC}

.include <bsd.prog.mk>

# $NetBSD: Makefile,v 1.2 1999/07/03 06:26:26 itojun Exp $

PROG=	traceroute6
MAN=	traceroute6.8

BINOWN=	root
BINGRP=	bin
BINMODE=4555

CPPFLAGS+=-DINET6 -DIPSEC
LDADD+=	-lipsec
DPADD+=	${LIBIPSEC}

.include <bsd.prog.mk>

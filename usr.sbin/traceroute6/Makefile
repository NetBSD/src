# $NetBSD: Makefile,v 1.1 1999/07/01 20:55:04 itojun Exp $

PROG=	traceroute6
MAN=	traceroute6.8

BINOWN=	root
BINGRP=	bin
BINMODE=4555

CFLAGS+=-DINET6 -DIPSEC
LDADD+=	-lipsec
DPADD+=	${LIBIPSEC}

.include <bsd.prog.mk>

# $NetBSD: Makefile,v 1.2 1999/07/01 20:19:35 itojun Exp $

PROG=	ping6
MAN=	ping6.8

BINOWN  = root
BINGRP  = bin
BINMODE = 4555

CFLAGS+= -DINET6
CFLAGS+= -DIPSEC
LDADD+=	-lipsec
DPADD+= ${LIBIPSEC}

.include <bsd.prog.mk>

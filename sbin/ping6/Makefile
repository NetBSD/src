# $NetBSD: Makefile,v 1.3 1999/07/02 00:04:53 thorpej Exp $

PROG=	ping6
MAN=	ping6.8

BINOWN  = root
BINGRP  = bin
BINMODE = 4555

CPPFLAGS+= -DINET6
CPPFLAGS+= -DIPSEC

LDADD+=	-lipsec
DPADD+= ${LIBIPSEC}

.include <bsd.prog.mk>

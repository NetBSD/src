# $NetBSD: Makefile,v 1.2 2001/06/26 17:10:33 itojun Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/5/93

PROG=	ifmcstat
MAN=	ifmcstat.8

BINGRP=	kmem
BINMODE=555
LDADD=	-lkvm
DPADD=	${LIBKVM}

CPPFLAGS+=	-DINET6

.include <bsd.prog.mk>

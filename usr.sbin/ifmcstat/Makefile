# $NetBSD: Makefile,v 1.1 1999/07/01 20:41:50 itojun Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/5/93

PROG=	ifmcstat
MAN=	ifmcstat.8

BINGRP=	kmem
BINMODE=2555
LDADD=	-lkvm
DPADD=	${LIBKVM}

CPPFLAGS+=	-DINET6

.include <bsd.prog.mk>

#	@(#)Makefile	8.1 (Berkeley) 6/6/93
#	$NetBSD: Makefile,v 1.7 1999/07/01 19:15:03 itojun Exp $

PROG=	trpt
MAN=	trpt.8

BINGRP=	kmem
BINMODE=2555

DPADD=	${LIBKVM}
LDADD=	-lkvm

CPPFLAGS+=	-DINET6

.include <bsd.prog.mk>

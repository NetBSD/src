#	$NetBSD: Makefile,v 1.5 1997/07/23 18:20:46 thorpej Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/6/93

PROG=	trsp
MAN=	trsp.8

BINGRP=	kmem
BINMODE=2555

DPADD=	${LIBKVM}
LDADD=	-lkvm

WARNS=1

.include <bsd.prog.mk>


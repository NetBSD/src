#	@(#)Makefile	8.1 (Berkeley) 6/6/93
#	$NetBSD: Makefile,v 1.5 1997/07/23 08:19:59 thorpej Exp $

PROG=	trpt
MAN=	trpt.8

BINGRP=	kmem
BINMODE=2555

DPADD=	${LIBKVM}
LDADD=	-lkvm

WARNS=1

.include <bsd.prog.mk>

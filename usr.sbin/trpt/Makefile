#	@(#)Makefile	8.1 (Berkeley) 6/6/93
#	$NetBSD: Makefile,v 1.8 2005/01/10 02:59:00 lukem Exp $

.include <bsd.own.mk>

PROG=	trpt
MAN=	trpt.8

BINGRP=	kmem
BINMODE=2555

DPADD=	${LIBKVM}
LDADD=	-lkvm

.if (${USE_INET6} != "no")
CPPFLAGS+=	-DINET6
.endif

.include <bsd.prog.mk>

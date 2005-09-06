#	@(#)Makefile	8.1 (Berkeley) 6/6/93
#	$NetBSD: Makefile,v 1.9 2005/09/06 03:05:15 rpaulo Exp $

.include <bsd.own.mk>

PROG=	trpt
MAN=	trpt.8

DPADD=	${LIBKVM}
LDADD=	-lkvm

.if (${USE_INET6} != "no")
CPPFLAGS+=	-DINET6
.endif

.include <bsd.prog.mk>

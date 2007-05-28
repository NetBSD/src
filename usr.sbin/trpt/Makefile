#	@(#)Makefile	8.1 (Berkeley) 6/6/93
#	$NetBSD: Makefile,v 1.10 2007/05/28 12:06:42 tls Exp $

.include <bsd.own.mk>

USE_FORT?= yes	# network client, effectively

PROG=	trpt
MAN=	trpt.8

DPADD=	${LIBKVM}
LDADD=	-lkvm

.if (${USE_INET6} != "no")
CPPFLAGS+=	-DINET6
.endif

.include <bsd.prog.mk>

#	$NetBSD: Makefile,v 1.2 2005/01/10 02:58:59 lukem Exp $

.include <bsd.own.mk>

PROG=	pvcsif
MAN=	pvcsif.8

.if (${USE_INET6} != "no")
CPPFLAGS+=-DINET6
.endif

.include <bsd.prog.mk>

#	$NetBSD: Makefile,v 1.2 2005/08/10 20:32:57 rpaulo Exp $

.include <bsd.own.mk>

PROG=		sockstat

.if (${USE_INET6} != "no")
CPPFLAGS+=-DINET6
.endif

.include <bsd.prog.mk>

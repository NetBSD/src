#	$NetBSD: Makefile,v 1.1.2.1 2005/10/06 11:36:28 tron Exp $

.include <bsd.own.mk>

PROG=		sockstat

.if (${USE_INET6} != "no")
CPPFLAGS+=-DINET6
.endif

.include <bsd.prog.mk>

#	$NetBSD: Makefile,v 1.3 2011/01/28 18:52:49 pooka Exp $

.include <bsd.own.mk>

RUMPPRG=	sockstat

.PATH:  ${.CURDIR}/../../lib/libc/gen
CPPFLAGS+= -DRUMP_ACTION
RUMPSRCS+=	sysctlgetmibinfo.c sysctlnametomib.c

.if (${USE_INET6} != "no")
CPPFLAGS+=-DINET6
.endif

.include <bsd.prog.mk>

#	$NetBSD: Makefile,v 1.2.40.1 2011/02/08 16:20:12 bouyer Exp $

.include <bsd.own.mk>

RUMPPRG=	sockstat

.PATH:  ${.CURDIR}/../../lib/libc/gen
CPPFLAGS+= -DRUMP_ACTION
RUMPSRCS+=	sysctlgetmibinfo.c sysctlnametomib.c

.if (${USE_INET6} != "no")
CPPFLAGS+=-DINET6
.endif

.include <bsd.prog.mk>

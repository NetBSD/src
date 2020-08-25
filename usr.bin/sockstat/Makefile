#	$NetBSD: Makefile,v 1.6 2020/08/25 16:39:16 christos Exp $

.include <bsd.own.mk>

RUMPPRG=	sockstat
BINGRP= kmem
BINMODE=2555
USE_FORT?= yes # setgid

.PATH:  ${.CURDIR}/../../lib/libc/gen
CPPFLAGS+= -DRUMP_ACTION
RUMPSRCS+=	sysctlgetmibinfo.c sysctlnametomib.c

SANITIZER_RENAME_CLASSES+=	rump
SANITIZER_RENAME_FILES.rump+=	${PROG}_rumpops.c ${RUMPSRCS}
SANITIZER_RENAME_SYMBOL.rump+=	sysctlgetmibinfo sysctlnametomib

.if (${USE_INET6} != "no")
CPPFLAGS+=-DINET6
.endif

.include <bsd.prog.mk>

#	$NetBSD: Makefile,v 1.33 2016/03/06 18:01:28 christos Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/6/93

.include <bsd.own.mk>
USE_FORT?= yes	# setgid
PROG=	vmstat

CPPFLAGS+=-D_KMEMUSER -I${NETBSDSRCDIR}/sys
SRCS=	drvstats.c vmstat.c
MAN=	vmstat.1
DPADD=	${LIBKVM}
LDADD=	-lkvm
BINGRP=	kmem
BINMODE=2555

CWARNFLAGS.clang+=	-Wno-format-extra-args
COPTS.vmstat.c += -Wno-format-nonliteral

# sparc64 is broken!
COPTS.vmstat.c += -Wno-error=stack-protector

.include <bsd.prog.mk>

#	$NetBSD: Makefile,v 1.31 2014/06/03 21:31:54 joerg Exp $
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
COPTS.vmstat.c += -Wno-format-nonliteral -Wno-shadow

.include <bsd.prog.mk>

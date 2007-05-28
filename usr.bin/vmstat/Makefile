#	$NetBSD: Makefile,v 1.26 2007/05/28 12:06:32 tls Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/6/93

USE_FORT?= yes	# setgid
PROG=	vmstat
WARNS=4

SRCS=	drvstats.c vmstat.c
MAN=	vmstat.1
DPADD=	${LIBKVM}
LDADD=	-lkvm
BINGRP=	kmem
BINMODE=2555

.include <bsd.prog.mk>

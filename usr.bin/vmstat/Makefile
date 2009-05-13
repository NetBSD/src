#	$NetBSD: Makefile,v 1.26.20.1 2009/05/13 19:20:11 jym Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/6/93

USE_FORT?= yes	# setgid
PROG=	vmstat

SRCS=	drvstats.c vmstat.c
MAN=	vmstat.1
DPADD=	${LIBKVM}
LDADD=	-lkvm
BINGRP=	kmem
BINMODE=2555

.include <bsd.prog.mk>

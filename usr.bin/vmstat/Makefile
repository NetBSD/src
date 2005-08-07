#	$NetBSD: Makefile,v 1.22 2005/08/07 12:23:20 blymn Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/6/93

PROG=	vmstat
WARNS=3

SRCS=	dkstats.c tpstats.c vmstat.c
MAN=	vmstat.1
DPADD=	${LIBKVM}
LDADD=	-lkvm
BINGRP=	kmem
BINMODE=2555

.include <bsd.prog.mk>

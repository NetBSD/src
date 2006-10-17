#	$NetBSD: Makefile,v 1.25 2006/10/17 15:13:08 christos Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/6/93

PROG=	vmstat
WARNS=4

SRCS=	drvstats.c vmstat.c
MAN=	vmstat.1
DPADD=	${LIBKVM}
LDADD=	-lkvm
BINGRP=	kmem
BINMODE=2555

.include <bsd.prog.mk>

#	$NetBSD: Makefile,v 1.22.2.1 2006/04/22 02:54:47 simonb Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/6/93

PROG=	vmstat
WARNS=3

SRCS=	drvstats.c vmstat.c
MAN=	vmstat.1
DPADD=	${LIBKVM}
LDADD=	-lkvm
BINGRP=	kmem
BINMODE=2555

.include <bsd.prog.mk>

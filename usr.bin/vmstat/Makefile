#	$NetBSD: Makefile,v 1.24 2006/06/07 20:58:23 kardel Exp $
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

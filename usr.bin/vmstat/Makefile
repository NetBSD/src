#	$NetBSD: Makefile,v 1.20 2001/05/06 23:37:36 wiz Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/6/93

PROG=	vmstat

SRCS=	dkstats.c vmstat.c
MAN=	vmstat.1
DPADD=	${LIBKVM}
LDADD=	-lkvm
BINGRP=	kmem
BINMODE=2555

.include <bsd.prog.mk>

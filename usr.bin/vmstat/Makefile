#	$NetBSD: Makefile,v 1.19 1999/09/12 01:19:26 chs Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/6/93

PROG=	vmstat

SRCS=	dkstats.c vmstat.c
MAN=	vmstat.8
DPADD=	${LIBKVM}
LDADD=	-lkvm
BINGRP=	kmem
BINMODE=2555

.include <bsd.prog.mk>

#	$NetBSD: Makefile,v 1.6 1996/06/07 00:44:18 thorpej Exp $
#	from: @(#)Makefile	5.6 (Berkeley) 4/23/91

PROG=	slstats
SRCS=	slstats.c
MAN=	slstats.8
DPADD=	${LIBKVM}
LDADD=	-lkvm
BINGRP=	kmem
BINMODE=2555

.include <bsd.prog.mk>

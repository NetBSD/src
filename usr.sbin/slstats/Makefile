#	$NetBSD: Makefile,v 1.5.6.1 1996/06/07 01:42:18 thorpej Exp $
#	from: @(#)Makefile	5.6 (Berkeley) 4/23/91

PROG=	slstats
SRCS=	slstats.c
MAN=	slstats.8
DPADD=	${LIBKVM}
LDADD=	-lkvm
BINGRP=	kmem
BINMODE=2555

.include <bsd.prog.mk>

#	$NetBSD: Makefile,v 1.7 1997/07/14 08:33:35 thorpej Exp $
#	from: @(#)Makefile	8.1 (Berkeley) 6/6/93

SRCS=	pstat.c swaplist.c
PROG=	pstat
MAN=	pstat.8
BINGRP=	kmem
BINMODE=2555
DPADD=	${LIBKVM}
LDADD=	-lkvm

CFLAGS+=-I${.CURDIR}/../../sbin/swapctl

.PATH: ${.CURDIR}/../../sbin/swapctl

.include <bsd.prog.mk>

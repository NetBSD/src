#	$NetBSD: Makefile,v 1.11 1999/09/12 01:19:26 chs Exp $
#	from: @(#)Makefile	8.1 (Berkeley) 6/6/93

SRCS=	pstat.c swaplist.c
PROG=	pstat
MAN=	pstat.8
BINGRP=	kmem
BINMODE=2555
DPADD=	${LIBKVM}
LDADD=	-lkvm

CPPFLAGS+=-I${.CURDIR}/../../sbin/swapctl

.PATH: ${.CURDIR}/../../sbin/swapctl

.include <bsd.prog.mk>

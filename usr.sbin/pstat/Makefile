#	$NetBSD: Makefile,v 1.12 2002/09/18 03:54:35 lukem Exp $
#	from: @(#)Makefile	8.1 (Berkeley) 6/6/93

.include <bsd.own.mk>

SRCS=	pstat.c swaplist.c
PROG=	pstat
MAN=	pstat.8
BINGRP=	kmem
BINMODE=2555
DPADD=	${LIBKVM}
LDADD=	-lkvm

CPPFLAGS+=-I${NETBSDSRCDIR}/sbin/swapctl

.PATH: ${NETBSDSRCDIR}/sbin/swapctl

.include <bsd.prog.mk>

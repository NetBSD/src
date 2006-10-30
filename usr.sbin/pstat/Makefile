#	$NetBSD: Makefile,v 1.13 2006/10/30 20:22:54 christos Exp $
#	from: @(#)Makefile	8.1 (Berkeley) 6/6/93

.include <bsd.own.mk>

SRCS=	pstat.c swaplist.c
PROG=	pstat
MAN=	pstat.8
BINGRP=	kmem
BINMODE=2555
DPADD=	${LIBKVM}
LDADD=	-lkvm

CPPFLAGS+=-I${NETBSDSRCDIR}/sbin/swapctl -DDEBUG -DLOCKDEBUG

.PATH: ${NETBSDSRCDIR}/sbin/swapctl

.include <bsd.prog.mk>

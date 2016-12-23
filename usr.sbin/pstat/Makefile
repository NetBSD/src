#	$NetBSD: Makefile,v 1.17 2016/12/23 10:19:57 mrg Exp $
#	from: @(#)Makefile	8.1 (Berkeley) 6/6/93

.include <bsd.own.mk>

USE_FORT?= yes	# setgid

SRCS=	pstat.c swaplist.c
PROG=	pstat
MAN=	pstat.8
BINGRP=	kmem
BINMODE=2555
DPADD=	${LIBKVM}
LDADD=	-lkvm

CPPFLAGS+=-I${NETBSDSRCDIR}/sbin/swapctl

CPPFLAGS+=-D_KMEMUSER

.PATH: ${NETBSDSRCDIR}/sbin/swapctl

.include "../../compat/exec.mk"

.include <bsd.prog.mk>

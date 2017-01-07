#	$NetBSD: Makefile,v 1.16.12.1 2017/01/07 08:57:00 pgoyette Exp $
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

#	$NetBSD: Makefile,v 1.9 1998/02/09 06:25:19 mrg Exp $
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

# XXX
.if defined(UVM)
CPPFLAGS+=      -DUVM
.endif

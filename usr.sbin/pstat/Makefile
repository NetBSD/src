#	$NetBSD: Makefile,v 1.6 1997/06/12 15:03:43 mrg Exp $
#	from: @(#)Makefile	8.1 (Berkeley) 6/6/93

SRCS=	pstat.c swaplist.c
PROG=	pstat
MAN=	pstat.8
BINGRP=	kmem
BINMODE=2555
DPADD=	${LIBKVM}
LDADD=	-lkvm
.if (${MACHINE_ARCH} == "m68k")
CFLAGS+=-D${MACHINE}
.endif
CFLAGS+=-I${.CURDIR}/../../sbin/swapctl

.PATH: ${.CURDIR}/../../sbin/swapctl

.include <bsd.prog.mk>

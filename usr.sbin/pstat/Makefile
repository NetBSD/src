#	$NetBSD: Makefile,v 1.5.2.1 1997/06/01 09:05:28 mrg Exp $
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
CFLAGS+=-I${.CURDIR}/../../sbin/swap

.PATH: ${.CURDIR}/../../sbin/swap

.include <bsd.prog.mk>

#	$NetBSD: Makefile,v 1.5 1996/01/11 06:33:32 thorpej Exp $
#	from: @(#)Makefile	8.1 (Berkeley) 6/6/93

PROG=	pstat
MAN=	pstat.8
BINGRP=	kmem
BINMODE=2555
DPADD=	${LIBKVM}
LDADD=	-lkvm
.if (${MACHINE_ARCH} == "m68k")
CFLAGS+=-D${MACHINE}
.endif

.include <bsd.prog.mk>

#	from: @(#)Makefile	8.1 (Berkeley) 6/6/93
#	$Id: Makefile,v 1.8 1994/05/21 10:12:29 chopps Exp $

PROG=	vmstat
.if (${MACHINE_ARCH} == "m68k")
CFLAGS+=-D${MACHINE}
.endif
MAN8=	vmstat.0
BINGRP=	kmem
BINMODE=2555
DPADD=	names.c ${LIBKVM}
LDADD=	-lkvm

.include <bsd.prog.mk>

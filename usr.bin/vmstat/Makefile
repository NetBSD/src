#	from: @(#)Makefile	8.1 (Berkeley) 6/6/93
#	$Id: Makefile,v 1.12 1994/12/22 12:31:30 cgd Exp $

PROG=	vmstat
.if (${MACHINE_ARCH} == "m68k")
CFLAGS+=-D${MACHINE}
.endif
MAN=	vmstat.8
BINGRP=	kmem
BINMODE=2555
DPADD=	names.c ${LIBKVM}
LDADD=	-lkvm

.include <bsd.prog.mk>

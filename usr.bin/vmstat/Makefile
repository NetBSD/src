#	from: @(#)Makefile	8.1 (Berkeley) 6/6/93
#	$Id: Makefile,v 1.9 1994/05/30 17:10:16 hpeyerl Exp $

PROG=	vmstat
.if (${MACHINE_ARCH} == "m68k")
CFLAGS+=-D${MACHINE} -I../../../sys/arch
.endif
MAN8=	vmstat.0
BINGRP=	kmem
BINMODE=2555
DPADD=	names.c ${LIBKVM}
LDADD=	-lkvm

.include <bsd.prog.mk>

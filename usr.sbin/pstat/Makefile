#	from: @(#)Makefile	8.1 (Berkeley) 6/6/93
#	$Id: Makefile,v 1.4 1994/12/25 12:22:03 pk Exp $

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

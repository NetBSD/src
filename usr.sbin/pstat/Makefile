#	from: @(#)Makefile	8.1 (Berkeley) 6/6/93
#	$Id: Makefile,v 1.3 1994/06/22 05:38:20 chopps Exp $

PROG=	pstat
BINGRP=	kmem
BINMODE=2555
DPADD=	${LIBKVM}
LDADD=	-lkvm
.if (${MACHINE_ARCH} == "m68k")
CFLAGS+=-D${MACHINE}
.endif

.include <bsd.prog.mk>

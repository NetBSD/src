#	from: @(#)Makefile	8.1 (Berkeley) 6/6/93
#	$Id: Makefile,v 1.13 1994/12/28 18:39:53 mycroft Exp $

PROG=	vmstat
.if (${MACHINE_ARCH} == "m68k")
CFLAGS+=-D${MACHINE} -I${.CURDIR}/../../sys/arch
.endif
MAN=	vmstat.8
BINGRP=	kmem
BINMODE=2555
DPADD=	names.c ${LIBKVM}
LDADD=	-lkvm

.include <bsd.prog.mk>

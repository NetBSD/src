#	$NetBSD: Makefile,v 1.12 1995/12/22 08:04:27 jonathan Exp $
#	from: @(#)Makefile	8.1 (Berkeley) 6/6/93

PROG=	iostat
.if (${MACHINE_ARCH} == "m68k")
CFLAGS+=-D${MACHINE}
.endif

CFLAGS+= -I${.CURDIR}/../../sys/arch -I${.CURDIR}/../../usr.bin/vmstat
MAN=	iostat.8
DPADD=	${LIBKVM}
LDADD=	-lkvm
BINGRP=	kmem
BINMODE=2555

.include <bsd.prog.mk>

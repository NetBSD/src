#	$NetBSD: Makefile,v 1.17 2001/02/04 14:42:31 mrg Exp $
#	from: @(#)Makefile	8.1 (Berkeley) 6/6/93

PROG=	iostat
MAN=	iostat.8

.if ${MACHINE} == "sparc64"
CFLAGS+=	-msoft-quad-float
.endif

.PATH:	${.CURDIR}/../../usr.bin/vmstat

CPPFLAGS+=-I${.CURDIR}/../../usr.bin/vmstat

# dkstats.c pulled in from ../../usr.bin/vmstat
SRCS=	dkstats.c iostat.c

DPADD=	${LIBKVM}
LDADD=	-lkvm
BINGRP=	kmem
BINMODE=2555

.include <bsd.prog.mk>

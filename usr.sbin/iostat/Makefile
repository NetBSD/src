#	$NetBSD: Makefile,v 1.15 1997/05/08 21:12:14 gwr Exp $
#	from: @(#)Makefile	8.1 (Berkeley) 6/6/93

PROG=	iostat
MAN=	iostat.8

.PATH:	${.CURDIR}/../../usr.bin/vmstat

CFLAGS+=-I${.CURDIR}/../../usr.bin/vmstat

# dkstats.c pulled in from ../../usr.bin/vmstat
SRCS=	dkstats.c iostat.c

DPADD=	${LIBKVM}
LDADD=	-lkvm
BINGRP=	kmem
BINMODE=2555

.include <bsd.prog.mk>

#	from: @(#)Makefile	5.8 (Berkeley) 7/28/90
#	$NetBSD: Makefile,v 1.4 1997/09/24 07:22:39 jeremy Exp $

.if ${MACHINE} == "sun3" || ${MACHINE} == "sun3x" || ${MACHINE} == "sparc"
PROG=	eeprom

YFLAGS+=-pgetdate_

BINGRP=	kmem
BINMODE=2555

SRCS=	eehandlers.c getdate.c main.c

.if ${MACHINE} == "sparc"
SRCS+=	ophandlers.c
DPADD=	${LIBKVM}
LDADD=	-lkvm
.endif

CLEANFILES+=getdate.c y.tab.h
.endif

MAN=	eeprom.8

WARNS=1

.include <bsd.prog.mk>

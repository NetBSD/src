#	from: @(#)Makefile	5.8 (Berkeley) 7/28/90
#	$NetBSD: Makefile,v 1.2 1997/06/23 04:51:21 mrg Exp $

.if ${MACHINE} == "sun3" || ${MACHINE} == "sparc"
PROG=	eeprom

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

.include <bsd.prog.mk>

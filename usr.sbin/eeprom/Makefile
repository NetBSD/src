#	from: @(#)Makefile	5.8 (Berkeley) 7/28/90
#	$NetBSD: Makefile,v 1.7 1998/11/01 03:51:16 lukem Exp $

.if ${MACHINE} == "sun3" || ${MACHINE} == "sun3x" || ${MACHINE} == "sparc"
PROG=	eeprom

YPREFIX=getdate_

BINGRP=	kmem
BINMODE=2555

SRCS=	eehandlers.c getdate.y main.c

.if ${MACHINE} == "sparc"
SRCS+=	ophandlers.c
DPADD=	${LIBKVM}
LDADD=	-lkvm
.endif
.endif

MAN=	eeprom.8

.include <bsd.prog.mk>

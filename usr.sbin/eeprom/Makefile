#	from: @(#)Makefile	5.8 (Berkeley) 7/28/90
#	$NetBSD: Makefile,v 1.8 1999/01/17 20:02:29 eeh Exp $

.if ${MACHINE} == "sun3" || ${MACHINE} == "sun3x" || ${MACHINE} == "sparc" \
	|| ${MACHINE} == "sparc64"
PROG=	eeprom

YPREFIX=getdate_

BINGRP=	kmem
BINMODE=2555

SRCS=	eehandlers.c getdate.y main.c

.if ${MACHINE} == "sparc" || ${MACHINE} == "sparc64"
SRCS+=	ophandlers.c
DPADD=	${LIBKVM}
LDADD=	-lkvm
.endif
.endif

MAN=	eeprom.8

.include <bsd.prog.mk>

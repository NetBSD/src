#	from: @(#)Makefile	5.8 (Berkeley) 7/28/90
#	$NetBSD: Makefile,v 1.6 1998/04/09 00:32:40 tv Exp $

.if ${MACHINE} == "sun3" || ${MACHINE} == "sun3x" || ${MACHINE} == "sparc"
PROG=	eeprom

YFLAGS+=-pgetdate_

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

#	from: @(#)Makefile	5.8 (Berkeley) 7/28/90
#	$NetBSD: Makefile,v 1.11 2003/05/08 20:24:10 petrov Exp $

.if ${MACHINE} == "sun3" || ${MACHINE} == "sun3x" || ${MACHINE} == "sparc" \
	|| ${MACHINE} == "sparc64"
PROG=	eeprom

YPREFIX=getdate_

SRCS=	getdate.y main.c

.if ${MACHINE} == "sparc" || ${MACHINE} == "sparc64"
SRCS+=	ophandlers.c
.endif
.if ${MACHINE_ARCH} != "sparc64"
SRCS+=	eehandlers.c
.endif

.endif

MAN=	eeprom.8

CPPFLAGS+= -I${.CURDIR}

.include <bsd.prog.mk>

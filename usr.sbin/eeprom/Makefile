#	from: @(#)Makefile	5.8 (Berkeley) 7/28/90
#	$NetBSD: Makefile,v 1.9 2000/11/28 22:31:37 mrg Exp $

.if ${MACHINE} == "sun3" || ${MACHINE} == "sun3x" || ${MACHINE} == "sparc" \
	|| ${MACHINE} == "sparc64"
PROG=	eeprom

YPREFIX=getdate_

SRCS=	getdate.y main.c

.if ${MACHINE} == "sparc" || ${MACHINE} == "sparc64"
SRCS+=	ophandlers.c
.endif
.if ${MACHINE} != "sparc64"
SRCS+=	eehandlers.c
.endif

.endif

MAN=	eeprom.8

.include <bsd.prog.mk>

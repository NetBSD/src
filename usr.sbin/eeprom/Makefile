#	from: @(#)Makefile	5.8 (Berkeley) 7/28/90
#	$NetBSD: Makefile,v 1.16.6.1 2012/04/17 00:09:46 yamt Exp $

.if ${MACHINE} == "sun3" || ${MACHINE} == "sun3x" \
	|| ${MACHINE} == "sparc" || ${MACHINE} == "sparc64" \
	|| ${MACHINE} == "macppc" || ${MACHINE} == "prep" \
	|| ${MACHINE} == "shark"
PROG=	eeprom

SRCS=	main.c
LDADD+=-lutil
DPADD+=${LIBUTIL}

.if ${MACHINE} == "sparc" || ${MACHINE} == "sparc64"
SRCS+=	ophandlers.c
.endif

.if ${MACHINE} == "sun3" || ${MACHINE} == "sun3x" || ${MACHINE} == "sparc" 
SRCS+=	eehandlers.c
.endif

.if ${MACHINE} == "macppc" || ${MACHINE} == "shark"
SRCS+=		ofhandlers.c
CPPFLAGS+=	-DUSE_OPENFIRM
.endif

.if ${MACHINE} == "prep"
SRCS+=		prephandlers.c
CPPFLAGS+=	-DUSE_PREPNVRAM
.endif

.endif

MAN=	eeprom.8

CPPFLAGS+= -I${.CURDIR}

.include <bsd.prog.mk>

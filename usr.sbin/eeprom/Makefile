#	from: @(#)Makefile	5.8 (Berkeley) 7/28/90
#	$NetBSD: Makefile,v 1.16 2009/12/13 05:01:33 nakayama Exp $

.if ${MACHINE} == "sun3" || ${MACHINE} == "sun3x" \
	|| ${MACHINE} == "sparc" || ${MACHINE} == "sparc64" \
	|| ${MACHINE} == "macppc" || ${MACHINE} == "prep"
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

.if ${MACHINE} == "macppc"
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

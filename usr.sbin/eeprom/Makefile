#	from: @(#)Makefile	5.8 (Berkeley) 7/28/90
#	$NetBSD: Makefile,v 1.20 2026/09/21 08:01:08 macallan Exp $

.include <bsd.own.mk>

.if ${MACHINE} == "sun3" || ${MACHINE} == "sun3x" \
	|| ${MACHINE} == "sparc" || ${MACHINE} == "sparc64" \
	|| ${MACHINE} == "macppc" || ${MACHINE} == "prep" \
	|| ${MACHINE_CPU} == "arm" || ${MACHINE} == "ofppc"
PROG=	eeprom

SRCS=	main.c
LDADD+=-lutil
DPADD+=${LIBUTIL}

.if ${MACHINE} == "sparc" || ${MACHINE} == "sparc64"
SRCS+=	ophandlers.c
CPPFLAGS+=	-DUSE_OPENPROM
.endif

.if ${MACHINE} == "sun3" || ${MACHINE} == "sun3x" || ${MACHINE} == "sparc" 
SRCS+=	eehandlers.c
CPPFLAGS+=	-DUSE_EEPROM
.endif

.if ${MACHINE} == "macppc" || ${MACHINE_CPU} == "arm" || ${MACHINE} == "ofppc"
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

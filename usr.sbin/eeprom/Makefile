#	from: @(#)Makefile	5.8 (Berkeley) 7/28/90
#	$NetBSD: Makefile,v 1.14.20.1 2009/05/13 19:20:21 jym Exp $

.if ${MACHINE} == "sun3" || ${MACHINE} == "sun3x" \
	|| ${MACHINE_ARCH} == "sparc" || ${MACHINE_ARCH} == "sparc64" \
	|| ${MACHINE} == "macppc" || ${MACHINE} == "prep"
PROG=	eeprom

SRCS=	main.c
LDADD+=-lutil
DPADD+=${LIBUTIL}

.if ${MACHINE_ARCH} == "sparc" || ${MACHINE_ARCH} == "sparc64"
SRCS+=	ophandlers.c
.endif

.if ${MACHINE} == "sun3" || ${MACHINE} == "sun3x" || ${MACHINE_ARCH} == "sparc" 
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

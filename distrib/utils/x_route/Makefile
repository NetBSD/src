#	$NetBSD: Makefile,v 1.9.4.1 2000/09/07 01:36:58 minoura Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/5/93

PROG=	route
MKMAN=	no
SRCS=	route.c show.c keywords.c

SRCDIR= ${.CURDIR}/../../../sbin/route
CPPFLAGS+= -DSMALL -I${SRCDIR}
.if (${MACHINE} == "i386") || (${MACHINE} == "hpcmips") || (${MACHINE} == "x68k")
CPPFLAGS+= -DINET6
.endif
COPTS+= -Os
LDSTATIC?=-static

BINOWN=	root
BINMODE=4555

all: ${PROG}

.include <bsd.prog.mk>

.PATH:  ${SRCDIR}

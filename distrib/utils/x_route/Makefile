#	$NetBSD: Makefile,v 1.2 1996/11/15 19:18:39 gwr Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/5/93

PROG=	route
NOMAN=
SRCS=	route.c show.c keywords.c

SRCDIR= ${.CURDIR}/../../../sbin/route
CFLAGS+= -DSMALL -I${SRCDIR}

BINOWN=	root
BINMODE=4555

all: ${PROG}

.include <bsd.prog.mk>

.PATH:  ${SRCDIR}

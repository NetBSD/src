#	$NetBSD: Makefile,v 1.3 1997/10/22 02:20:09 lukem Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/5/93

PROG=	route
NOMAN=
SRCS=	route.c show.c keywords.c

SRCDIR= ${.CURDIR}/../../../sbin/route
CPPFLAGS+= -DSMALL -I${SRCDIR}

BINOWN=	root
BINMODE=4555

all: ${PROG}

.include <bsd.prog.mk>

.PATH:  ${SRCDIR}

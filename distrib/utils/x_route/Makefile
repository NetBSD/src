#	$NetBSD: Makefile,v 1.2.2.1 1997/11/08 05:17:53 lukem Exp $
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

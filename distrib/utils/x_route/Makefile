#	$NetBSD: Makefile,v 1.5 1999/05/19 04:17:50 gwr Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/5/93

PROG=	route
MKMAN=	no
SRCS=	route.c show.c keywords.c

SRCDIR= ${BSDSRCDIR}/sbin/route
CPPFLAGS+= -DSMALL -I${SRCDIR}

BINOWN=	root
BINMODE=4555

all: ${PROG}

.include <bsd.prog.mk>

.PATH:  ${SRCDIR}

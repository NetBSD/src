#	$NetBSD: Makefile,v 1.6 1999/06/21 01:19:37 cgd Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/5/93

PROG=	route
MKMAN=	no
SRCS=	route.c show.c keywords.c

SRCDIR= ${.CURDIR}/../../../sbin/route
CPPFLAGS+= -DSMALL -I${SRCDIR}

BINOWN=	root
BINMODE=4555

all: ${PROG}

.include <bsd.prog.mk>

.PATH:  ${SRCDIR}

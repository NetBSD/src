#	$NetBSD: Makefile,v 1.1.1.1 1996/09/13 18:21:25 gwr Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/5/93

PROG=	route
NOMAN=
SRCS=	route.c show.c

# Uncomment one of these:
# SRCS+= ccitt_addr.c
CFLAGS+=-DSMALL

BINOWN=	root
BINMODE=4555

.include <bsd.prog.mk>

#	$NetBSD: Makefile,v 1.2 1997/04/19 17:09:53 christos Exp $

PROG=	ld
SRCS=	ld.c symbol.c lib.c shlib.c warnings.c etc.c rrs.c xbits.c md.c
LDADD+= -lgnumalloc
DPADD+= /usr/lib/libgnumalloc.a
BINDIR=/usr/bin

.include <bsd.prog.mk>

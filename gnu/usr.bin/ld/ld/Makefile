#	$NetBSD: Makefile,v 1.1 1997/04/16 16:49:41 christos Exp $

PROG=	ld
SRCS=	ld.c symbol.c lib.c shlib.c warnings.c etc.c rrs.c xbits.c md.c
LDADD+= -lgnumalloc
DPADD+= /usr/lib/libgnumalloc.a

.include <bsd.prog.mk>

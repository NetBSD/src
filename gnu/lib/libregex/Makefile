# $Header: /cvsroot/src/gnu/lib/libregex/Attic/Makefile,v 1.1 1993/04/08 01:11:28 cgd Exp $

LIB=	gnuregex
CFLAGS+=-DHAVE_STRING_H=1
SRCS=	regex.c
NOMAN=	noman

afterinstall:
	install -c -o root -g wheel -m 444 ${.CURDIR}/regex.h /usr/include

.include <bsd.lib.mk>

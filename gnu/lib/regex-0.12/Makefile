# $Header: /cvsroot/src/gnu/lib/regex-0.12/Attic/Makefile,v 1.2 1993/04/10 15:24:44 cgd Exp $

LIB=	gnuregex
CFLAGS+=-DHAVE_STRING_H=1
SRCS=	regex.c
NOMAN=	noman

afterinstall:
	install -c -o root -g wheel -m 444 ${.CURDIR}/regex.h \
		${DESTDIR}/usr/include

.include <bsd.lib.mk>

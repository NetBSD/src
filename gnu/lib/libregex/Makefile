#	$Id: Makefile,v 1.3 1993/08/02 17:24:43 mycroft Exp $

LIB=	gnuregex
CFLAGS+=-DHAVE_STRING_H=1
SRCS=	regex.c
NOMAN=	noman

afterinstall:
	install -c -o root -g wheel -m 444 ${.CURDIR}/regex.h \
		${DESTDIR}/usr/include

.include <bsd.lib.mk>

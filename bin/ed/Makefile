# $Header: /cvsroot/src/bin/ed/Makefile,v 1.2 1993/04/10 11:46:38 cgd Exp $

PROG   =ed
CFLAGS+=-DDES -DGNU_REGEX 
SRCS=	ed.c re.c buf.c cbc.c
LDADD= -lgnuregex
NOMAN=	noman

.include <bsd.prog.mk>

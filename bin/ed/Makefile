# $Header: /cvsroot/src/bin/ed/Makefile,v 1.1 1993/04/08 01:07:16 cgd Exp $

PROG   =ed
CFLAGS+=-I${.CURDIR} -DDES -W -DGNU_REGEX 
SRCS=	ed.c re.c buf.c cbc.c
LDADD=	-lgnuregex
NOMAN=	noman

.include <bsd.prog.mk>

#	$NetBSD: Makefile,v 1.4 1997/07/18 21:57:08 thorpej Exp $

PROG=	stdethers
SRCS=	read_line.c stdethers.c
MAN=	stdethers.8
.PATH:	${.CURDIR}/../common
CFLAGS+=-I${.CURDIR}/../common
CFLAGS+= -DNTOA_FIX
WARNS=1

.include <bsd.prog.mk>

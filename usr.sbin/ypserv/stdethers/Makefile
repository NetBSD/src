#	$NetBSD: Makefile,v 1.1.1.1 1996/08/09 10:14:57 thorpej Exp $

PROG=	stdethers
SRCS=	read_line.c stdethers.c
MAN=	stdethers.8
.PATH:	${.CURDIR}/../common
CFLAGS+=-I${.CURDIR}/../common
CFLAGS+= -DNTOA_FIX

.include <bsd.prog.mk>

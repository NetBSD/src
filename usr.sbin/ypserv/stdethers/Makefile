#	$NetBSD: Makefile,v 1.3 1997/05/08 21:12:50 gwr Exp $

PROG=	stdethers
SRCS=	read_line.c stdethers.c
MAN=	stdethers.8
.PATH:	${.CURDIR}/../common
CFLAGS+=-I${.CURDIR}/../common
CFLAGS+= -DNTOA_FIX

.include <bsd.prog.mk>

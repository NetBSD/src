#	$NetBSD: Makefile,v 1.2 1997/05/06 20:46:44 gwr Exp $

PROG=	stdethers
SRCS=	read_line.c stdethers.c
MAN=	stdethers.8
.PATH.c: ${.CURDIR}/../common
CFLAGS+=-I${.CURDIR}/../common
CFLAGS+= -DNTOA_FIX

.include <bsd.prog.mk>

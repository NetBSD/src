#	$NetBSD: Makefile,v 1.4 1997/07/18 21:57:09 thorpej Exp $

PROG=	stdhosts
SRCS=	read_line.c stdhosts.c
MAN=	stdhosts.8
.PATH:	${.CURDIR}/../common
CFLAGS+=-I${.CURDIR}/../common
WARNS=1

.include <bsd.prog.mk>

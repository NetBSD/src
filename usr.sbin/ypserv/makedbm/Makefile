#	$NetBSD: Makefile,v 1.4 1997/07/18 21:57:04 thorpej Exp $

PROG=	makedbm
SRCS=	localhostname.c makedbm.c read_line.c ypdb.c
MAN=	makedbm.8
.PATH:	${.CURDIR}/../common
CFLAGS+=-I${.CURDIR}/../common
WARNS=1

.include <bsd.prog.mk>

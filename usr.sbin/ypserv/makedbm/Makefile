#	$NetBSD: Makefile,v 1.3 1997/05/08 21:12:48 gwr Exp $

PROG=	makedbm
SRCS=	localhostname.c makedbm.c read_line.c ypdb.c
MAN=	makedbm.8
.PATH:	${.CURDIR}/../common
CFLAGS+=-I${.CURDIR}/../common

.include <bsd.prog.mk>

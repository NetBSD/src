#	$NetBSD: Makefile,v 1.2 1997/05/06 20:46:42 gwr Exp $

PROG=	makedbm
SRCS=	localhostname.c makedbm.c read_line.c ypdb.c
MAN=	makedbm.8
.PATH.c: ${.CURDIR}/../common
CFLAGS+=-I${.CURDIR}/../common

.include <bsd.prog.mk>

#	$NetBSD: Makefile,v 1.1.1.1 1996/08/09 10:14:54 thorpej Exp $

PROG=	makedbm
SRCS=	localhostname.c makedbm.c read_line.c ypdb.c
MAN=	makedbm.8
.PATH:	${.CURDIR}/../common
CFLAGS+=-I${.CURDIR}/../common

.include <bsd.prog.mk>

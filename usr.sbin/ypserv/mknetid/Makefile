#	$NetBSD: Makefile,v 1.4 1997/07/18 21:57:06 thorpej Exp $

PROG=	mknetid
SRCS=	mknetid.c read_line.c
MAN=	mknetid.8 netid.5
.PATH:	${.CURDIR}/../common
CFLAGS+=-I${.CURDIR}/../common
WARNS=1

.include <bsd.prog.mk>

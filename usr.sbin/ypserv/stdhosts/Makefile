#	$NetBSD: Makefile,v 1.2 1997/05/06 20:46:45 gwr Exp $

PROG=	stdhosts
SRCS=	read_line.c stdhosts.c
MAN=	stdhosts.8
.PATH.c: ${.CURDIR}/../common
CFLAGS+=-I${.CURDIR}/../common

.include <bsd.prog.mk>

#	$NetBSD: Makefile,v 1.2 1997/05/06 20:46:43 gwr Exp $

PROG=	mknetid
SRCS=	mknetid.c read_line.c
MAN=	mknetid.8 netid.5
.PATH.c: ${.CURDIR}/../common
CFLAGS+=-I${.CURDIR}/../common

.include <bsd.prog.mk>

#	$NetBSD: Makefile,v 1.6 1997/05/08 21:12:05 gwr Exp $

.PATH:	${.CURDIR}/../lint1

PROG=	xlint
PROGNAME= lint
SRCS=	xlint.c mem.c
MAN=	lint.1

CFLAGS+=-I${.CURDIR}/../lint1

.include "${.CURDIR}/../../Makefile.inc"
.include <bsd.prog.mk>

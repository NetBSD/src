#	$NetBSD: Makefile,v 1.5 1997/05/06 20:45:58 gwr Exp $

.PATH.c: ${.CURDIR}/../lint1

PROG=	xlint
PROGNAME= lint
SRCS=	xlint.c mem.c
MAN=	lint.1

CFLAGS+=-I${.CURDIR}/../lint1

.include "${.CURDIR}/../../Makefile.inc"
.include <bsd.prog.mk>

#	$NetBSD: Makefile,v 1.4 1997/03/24 22:00:08 christos Exp $

.PATH:	${.CURDIR}/../lint1

PROG=	xlint
PROGNAME= lint
SRCS=	xlint.c mem.c
MAN=	lint.1

CFLAGS+=-I${.CURDIR}/../lint1

.include "${.CURDIR}/../../Makefile.inc"
.include <bsd.prog.mk>

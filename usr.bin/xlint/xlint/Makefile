#	$NetBSD: Makefile,v 1.9 2001/08/14 10:18:29 tv Exp $

.PATH:	${.CURDIR}/../lint1

PROG=	xlint
PROGNAME= lint
SRCS=	xlint.c mem.c
MAN=	lint.1

CPPFLAGS+=-I${.CURDIR}/../lint1

.ifndef HOSTPROG
.include <bsd.prog.mk>
.endif

#	$NetBSD: Makefile,v 1.13 2002/06/14 23:20:42 simonb Exp $

.PATH:		${.CURDIR}/../lint1
.PATH:		${.CURDIR}/../../mkdep

PROG=		xlint
SRCS=		xlint.c mem.c findcc.c
PROGNAME= 	lint
MAN=		lint.1

CPPFLAGS+=	-I${.CURDIR}/../lint1
CPPFLAGS+=	-I${.CURDIR}/../../mkdep

.ifndef HOSTPROG
.include <bsd.prog.mk>
.endif

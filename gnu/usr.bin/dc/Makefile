#	$NetBSD: Makefile,v 1.6 1999/01/16 05:57:50 simonb Exp $

TOP=	${.CURDIR}/../..
dist=	${TOP}/dist
srcdir=	${dist}/bc

PROG=	dc
CPPFLAGS+=-D_POSIX_SOURCE -I${.CURDIR}/../bc -I${srcdir}/bc -I${srcdir}/h
SRCS=	array.c dc.c eval.c misc.c numeric.c stack.c string.c \
	number.c getopt.c getopt1.c

.PATH:	${srcdir}/dc
.PATH:	${srcdir}/lib
.PATH:	${srcdir}/doc

.include <bsd.prog.mk>

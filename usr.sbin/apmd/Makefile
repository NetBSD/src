#	$NetBSD: Makefile,v 1.4 1997/10/03 05:39:05 mikel Exp $

.if ${MACHINE} == "i386"
PROG=	apmd
SRCS=	apmd.c apmsubr.c
WARNS?=	1
.endif

MAN=	apmd.8
MANSUBDIR=/i386

.include <bsd.prog.mk>

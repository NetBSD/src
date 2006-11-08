#	$NetBSD: Makefile,v 1.1 2006/11/08 21:01:36 christos Exp $

NOMAN=	#defined

.include <bsd.own.mk>

PROG=	raw
SRCS=	raw.c

regress: ${PROG}
	./${PROG} 9
	-./${PROG} 10

.include <bsd.prog.mk>

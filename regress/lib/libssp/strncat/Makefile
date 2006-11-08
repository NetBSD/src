#	$NetBSD: Makefile,v 1.1 2006/11/08 21:01:36 christos Exp $

NOMAN=	#defined

.include <bsd.own.mk>

PROG=	strncat
SRCS=	strncat.c

regress: ${PROG}
	./${PROG} 8
	-./${PROG} 9

.include <bsd.prog.mk>

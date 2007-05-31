#	$NetBSD: Makefile,v 1.1 2007/05/31 21:51:50 christos Exp $

NOMAN=	#defined

.include <bsd.own.mk>

PROG=	memmove
SRCS=	memmove.c

regress: ${PROG}
	./${PROG} 10
	-./${PROG} 11

.include <bsd.prog.mk>

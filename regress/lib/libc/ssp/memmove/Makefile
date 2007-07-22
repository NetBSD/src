#	$NetBSD: Makefile,v 1.1.2.2 2007/07/22 18:44:58 liamjfoy Exp $

NOMAN=	#defined

.include <bsd.own.mk>

PROG=	memmove
SRCS=	memmove.c

regress: ${PROG}
	./${PROG} 10
	-./${PROG} 11

.include <bsd.prog.mk>

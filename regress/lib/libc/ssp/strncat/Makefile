#	$NetBSD: Makefile,v 1.1.2.2 2007/07/22 18:45:07 liamjfoy Exp $

NOMAN=	#defined

.include <bsd.own.mk>

PROG=	strncat
SRCS=	strncat.c

regress: ${PROG}
	./${PROG} 8
	-./${PROG} 9

.include <bsd.prog.mk>

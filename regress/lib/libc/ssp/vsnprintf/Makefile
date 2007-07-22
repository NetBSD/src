#	$NetBSD: Makefile,v 1.1.2.2 2007/07/22 18:45:09 liamjfoy Exp $

NOMAN=	#defined

.include <bsd.own.mk>

PROG=	vsnprintf
SRCS=	vsnprintf.c

regress: ${PROG}
	./${PROG} 10
	-./${PROG} 11

.include <bsd.prog.mk>

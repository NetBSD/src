#	$NetBSD: Makefile,v 1.1 2007/05/31 21:51:54 christos Exp $

NOMAN=	#defined

.include <bsd.own.mk>

PROG=	strncat
SRCS=	strncat.c

regress: ${PROG}
	./${PROG} 8
	-./${PROG} 9

.include <bsd.prog.mk>

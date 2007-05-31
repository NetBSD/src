#	$NetBSD: Makefile,v 1.1 2007/05/31 21:51:54 christos Exp $

NOMAN=	#defined

.include <bsd.own.mk>

PROG=	strncpy
SRCS=	strncpy.c

regress: ${PROG}
	./${PROG} 10
	-./${PROG} 11

.include <bsd.prog.mk>

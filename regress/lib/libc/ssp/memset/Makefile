#	$NetBSD: Makefile,v 1.1 2007/05/31 21:51:51 christos Exp $

NOMAN=	#defined

.include <bsd.own.mk>

PROG=	memset
SRCS=	memset.c

regress: ${PROG}
	./${PROG} 10
	-./${PROG} 11

.include <bsd.prog.mk>

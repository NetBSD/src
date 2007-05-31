#	$NetBSD: Makefile,v 1.1 2007/05/31 21:51:51 christos Exp $

NOMAN=	#defined

.include <bsd.own.mk>

PROG=	raw
SRCS=	raw.c

regress: ${PROG}
	./${PROG} 9
	-./${PROG} 10

.include <bsd.prog.mk>

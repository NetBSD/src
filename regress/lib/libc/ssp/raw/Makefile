#	$NetBSD: Makefile,v 1.1.2.2 2007/07/22 18:45:00 liamjfoy Exp $

NOMAN=	#defined

.include <bsd.own.mk>

PROG=	raw
SRCS=	raw.c

regress: ${PROG}
	./${PROG} 9
	-./${PROG} 10

.include <bsd.prog.mk>

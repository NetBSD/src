#	$NetBSD: Makefile,v 1.1.2.2 2007/07/22 18:45:05 liamjfoy Exp $

NOMAN=	#defined

.include <bsd.own.mk>

PROG=	strcat
SRCS=	strcat.c

regress: ${PROG}
	./${PROG} 0123456
	-./${PROG} 012345678

.include <bsd.prog.mk>

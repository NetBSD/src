#	$NetBSD: Makefile,v 1.1.2.2 2007/07/22 18:45:06 liamjfoy Exp $

NOMAN=	#defined

.include <bsd.own.mk>

PROG=	strcpy
SRCS=	strcpy.c

regress: ${PROG}
	./${PROG} 0123456
	-./${PROG} 0123456789

.include <bsd.prog.mk>

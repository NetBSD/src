#	$NetBSD: Makefile,v 1.1.2.2 2007/07/22 18:45:04 liamjfoy Exp $

NOMAN=	#defined

.include <bsd.own.mk>

PROG=	sprintf
SRCS=	sprintf.c

regress: ${PROG}
	./${PROG} ok
	-./${PROG} 0123456789

.include <bsd.prog.mk>

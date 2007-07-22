#	$NetBSD: Makefile,v 1.1.2.2 2007/07/22 18:45:10 liamjfoy Exp $

NOMAN=	#defined

.include <bsd.own.mk>

PROG=	vsprintf
SRCS=	vsprintf.c

regress: ${PROG}
	./${PROG} ok
	-./${PROG} 0123456789

.include <bsd.prog.mk>

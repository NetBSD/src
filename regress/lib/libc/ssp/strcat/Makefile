#	$NetBSD: Makefile,v 1.1 2007/05/31 21:51:53 christos Exp $

NOMAN=	#defined

.include <bsd.own.mk>

PROG=	strcat
SRCS=	strcat.c

regress: ${PROG}
	./${PROG} 0123456
	-./${PROG} 012345678

.include <bsd.prog.mk>

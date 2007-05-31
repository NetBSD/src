#	$NetBSD: Makefile,v 1.1 2007/05/31 21:51:52 christos Exp $

NOMAN=	#defined

.include <bsd.own.mk>

PROG=	snprintf
SRCS=	snprintf.c

regress: ${PROG}
	./${PROG} 10
	-./${PROG} 11

.include <bsd.prog.mk>

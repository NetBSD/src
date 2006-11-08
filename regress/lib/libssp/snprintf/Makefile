#	$NetBSD: Makefile,v 1.1 2006/11/08 21:01:36 christos Exp $

NOMAN=	#defined

.include <bsd.own.mk>

PROG=	snprintf
SRCS=	snprintf.c

regress: ${PROG}
	./${PROG} 10
	-./${PROG} 11

.include <bsd.prog.mk>

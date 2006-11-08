#	$NetBSD: Makefile,v 1.1 2006/11/08 21:01:36 christos Exp $

NOMAN=	#defined

.include <bsd.own.mk>

PROG=	vsprintf
SRCS=	vsprintf.c

regress: ${PROG}
	./${PROG} ok
	-./${PROG} 0123456789

.include <bsd.prog.mk>

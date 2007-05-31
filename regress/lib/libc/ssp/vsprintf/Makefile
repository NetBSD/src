#	$NetBSD: Makefile,v 1.1 2007/05/31 21:51:55 christos Exp $

NOMAN=	#defined

.include <bsd.own.mk>

PROG=	vsprintf
SRCS=	vsprintf.c

regress: ${PROG}
	./${PROG} ok
	-./${PROG} 0123456789

.include <bsd.prog.mk>

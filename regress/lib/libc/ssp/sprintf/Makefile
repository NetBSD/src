#	$NetBSD: Makefile,v 1.1 2007/05/31 21:51:52 christos Exp $

NOMAN=	#defined

.include <bsd.own.mk>

PROG=	sprintf
SRCS=	sprintf.c

regress: ${PROG}
	./${PROG} ok
	-./${PROG} 0123456789

.include <bsd.prog.mk>

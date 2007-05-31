#	$NetBSD: Makefile,v 1.1 2007/05/31 21:51:49 christos Exp $

NOMAN=	#defined

.include <bsd.own.mk>

PROG=	getcwd
SRCS=	getcwd.c

regress: ${PROG}
	./${PROG} 1024
	-./${PROG} 1025

.include <bsd.prog.mk>

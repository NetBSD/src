#	$NetBSD: Makefile,v 1.1.2.2 2007/07/22 18:44:55 liamjfoy Exp $

NOMAN=	#defined

.include <bsd.own.mk>

PROG=	getcwd
SRCS=	getcwd.c

regress: ${PROG}
	./${PROG} 1024
	-./${PROG} 1025

.include <bsd.prog.mk>

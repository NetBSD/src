#	$NetBSD: Makefile,v 1.1.2.2 2007/07/22 18:45:02 liamjfoy Exp $

NOMAN=	#defined

.include <bsd.own.mk>

PROG=	readlink
SRCS=	readlink.c

regress: ${PROG}
	./${PROG} 1024
	-./${PROG} 1025

.include <bsd.prog.mk>

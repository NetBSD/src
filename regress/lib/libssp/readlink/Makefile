#	$NetBSD: Makefile,v 1.1 2006/11/08 21:01:36 christos Exp $

NOMAN=	#defined

.include <bsd.own.mk>

PROG=	readlink
SRCS=	readlink.c

regress: ${PROG}
	./${PROG} 1024
	-./${PROG} 1025

.include <bsd.prog.mk>

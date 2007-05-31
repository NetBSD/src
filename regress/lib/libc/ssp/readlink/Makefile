#	$NetBSD: Makefile,v 1.1 2007/05/31 21:51:52 christos Exp $

NOMAN=	#defined

.include <bsd.own.mk>

PROG=	readlink
SRCS=	readlink.c

regress: ${PROG}
	./${PROG} 1024
	-./${PROG} 1025

.include <bsd.prog.mk>

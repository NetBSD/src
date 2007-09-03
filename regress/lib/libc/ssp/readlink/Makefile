#	$NetBSD: Makefile,v 1.1.8.2 2007/09/03 07:02:36 wrstuden Exp $

NOMAN=	#defined

.include <bsd.own.mk>

PROG=	readlink
SRCS=	readlink.c

regress: ${PROG}
	./${PROG} 1024
	-./${PROG} 1025

.include <bsd.prog.mk>

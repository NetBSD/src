#	$NetBSD: Makefile,v 1.1.8.2 2007/09/03 07:02:31 wrstuden Exp $

NOMAN=	#defined

.include <bsd.own.mk>

PROG=	memcpy
SRCS=	memcpy.c

regress: ${PROG}
	./${PROG} 10
	-./${PROG} 11

.include <bsd.prog.mk>

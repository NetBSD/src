#	$NetBSD: Makefile,v 1.1.8.2 2007/09/03 07:02:34 wrstuden Exp $

NOMAN=	#defined

.include <bsd.own.mk>

PROG=	raw
SRCS=	raw.c

regress: ${PROG}
	./${PROG} 9
	-./${PROG} 10

.include <bsd.prog.mk>

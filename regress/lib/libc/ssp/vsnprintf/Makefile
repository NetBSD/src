#	$NetBSD: Makefile,v 1.1.8.2 2007/09/03 07:02:41 wrstuden Exp $

NOMAN=	#defined

.include <bsd.own.mk>

PROG=	vsnprintf
SRCS=	vsnprintf.c

regress: ${PROG}
	./${PROG} 10
	-./${PROG} 11

.include <bsd.prog.mk>

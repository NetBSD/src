#	$NetBSD: Makefile,v 1.1.8.2 2007/09/03 07:02:38 wrstuden Exp $

NOMAN=	#defined

.include <bsd.own.mk>

PROG=	strcat
SRCS=	strcat.c

regress: ${PROG}
	./${PROG} 0123456
	-./${PROG} 012345678

.include <bsd.prog.mk>

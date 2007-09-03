#	$NetBSD: Makefile,v 1.1.8.2 2007/09/03 07:02:39 wrstuden Exp $

NOMAN=	#defined

.include <bsd.own.mk>

PROG=	strcpy
SRCS=	strcpy.c

regress: ${PROG}
	./${PROG} 0123456
	-./${PROG} 0123456789

.include <bsd.prog.mk>

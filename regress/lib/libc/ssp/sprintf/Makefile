#	$NetBSD: Makefile,v 1.1.8.2 2007/09/03 07:02:38 wrstuden Exp $

NOMAN=	#defined

.include <bsd.own.mk>

PROG=	sprintf
SRCS=	sprintf.c

regress: ${PROG}
	./${PROG} ok
	-./${PROG} 0123456789

.include <bsd.prog.mk>

#	$NetBSD: Makefile,v 1.1.8.2 2007/09/03 07:02:29 wrstuden Exp $

NOMAN=	#defined

.include <bsd.own.mk>

PROG=	getcwd
SRCS=	getcwd.c

regress: ${PROG}
	./${PROG} 1024
	-./${PROG} 1025

.include <bsd.prog.mk>

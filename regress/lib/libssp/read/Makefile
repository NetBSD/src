#	$NetBSD: Makefile,v 1.1 2006/11/08 21:01:36 christos Exp $

NOMAN=	#defined

.include <bsd.own.mk>

PROG=	read
SRCS=	read.c

regress: ${PROG}
	echo foo | ./${PROG} 1024
	-(echo bar | ./${PROG} 1025)

.include <bsd.prog.mk>

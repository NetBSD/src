#	$NetBSD: Makefile,v 1.1.2.2 2007/07/22 18:45:01 liamjfoy Exp $

NOMAN=	#defined

.include <bsd.own.mk>

PROG=	read
SRCS=	read.c

regress: ${PROG}
	echo foo | ./${PROG} 1024
	-(echo bar | ./${PROG} 1025)

.include <bsd.prog.mk>

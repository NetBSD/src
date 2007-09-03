#	$NetBSD: Makefile,v 1.1.8.2 2007/09/03 07:02:35 wrstuden Exp $

NOMAN=	#defined

.include <bsd.own.mk>

PROG=	read
SRCS=	read.c

regress: ${PROG}
	echo foo | ./${PROG} 1024
	-(echo bar | ./${PROG} 1025)

.include <bsd.prog.mk>

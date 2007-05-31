#	$NetBSD: Makefile,v 1.1 2007/05/31 21:51:50 christos Exp $

NOMAN=	#defined

.include <bsd.own.mk>

PROG=	gets
SRCS=	gets.c

regress: ${PROG}
	echo ok | ./${PROG}
	-(echo 0123456789 | ./${PROG})

.include <bsd.prog.mk>

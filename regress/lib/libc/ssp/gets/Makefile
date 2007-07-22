#	$NetBSD: Makefile,v 1.1.2.2 2007/07/22 18:44:56 liamjfoy Exp $

NOMAN=	#defined

.include <bsd.own.mk>

PROG=	gets
SRCS=	gets.c

regress: ${PROG}
	echo ok | ./${PROG}
	-(echo 0123456789 | ./${PROG})

.include <bsd.prog.mk>

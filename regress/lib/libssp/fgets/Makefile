#	$NetBSD: Makefile,v 1.1 2006/11/08 21:01:36 christos Exp $

NOMAN=	#defined

.include <bsd.own.mk>

PROG=	fgets
SRCS=	fgets.c

regress: ${PROG}
	echo ok | ./${PROG} 10
	-(echo busted | ./${PROG} 11)

.include <bsd.prog.mk>

#	$NetBSD: Makefile,v 1.1 2007/05/31 21:51:49 christos Exp $

NOMAN=	#defined

.include <bsd.own.mk>

PROG=	fgets
SRCS=	fgets.c

regress: ${PROG}
	echo ok | ./${PROG} 10
	-(echo busted | ./${PROG} 11)

.include <bsd.prog.mk>

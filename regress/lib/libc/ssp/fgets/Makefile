#	$NetBSD: Makefile,v 1.1.2.2 2007/07/22 18:44:54 liamjfoy Exp $

NOMAN=	#defined

.include <bsd.own.mk>

PROG=	fgets
SRCS=	fgets.c

regress: ${PROG}
	echo ok | ./${PROG} 10
	-(echo busted | ./${PROG} 11)

.include <bsd.prog.mk>

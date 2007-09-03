#	$NetBSD: Makefile,v 1.1.8.2 2007/09/03 07:02:28 wrstuden Exp $

NOMAN=	#defined

.include <bsd.own.mk>

PROG=	fgets
SRCS=	fgets.c

regress: ${PROG}
	echo ok | ./${PROG} 10
	-(echo busted | ./${PROG} 11)

.include <bsd.prog.mk>

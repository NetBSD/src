#	$NetBSD: Makefile,v 1.1.8.2 2007/09/03 07:02:30 wrstuden Exp $

NOMAN=	#defined

.include <bsd.own.mk>

PROG=	gets
SRCS=	gets.c

regress: ${PROG}
	echo ok | ./${PROG}
	-(echo 0123456789 | ./${PROG})

.include <bsd.prog.mk>

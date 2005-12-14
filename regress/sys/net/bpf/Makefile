#	$NetBSD: Makefile,v 1.1 2005/12/14 23:55:02 rpaulo Exp $

NOMAN=	#defined

PROG=	ioctls
WARNS=	3

regress:
	@if ./${PROG} ; then		\
		echo "PASSED";		\
	else				\
		echo "FAILED";		\
	fi

.include <bsd.prog.mk>

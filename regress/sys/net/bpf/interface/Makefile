#	$NetBSD: Makefile,v 1.1 2005/12/25 22:07:01 rpaulo Exp $

NOMAN=	# defined

PROG=	interface
WARNS=	3

regress:
	@if ./${PROG} ; then		\
		echo "PASSED";		\
	else				\
		echo "FAILED";		\
	fi

.include <bsd.prog.mk>

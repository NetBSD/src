#	$NetBSD: Makefile,v 1.1 2002/08/28 07:47:20 gmcgarry Exp $

PROG=	ras1
NOMAN=	#defined
WARNS=	2

regress:
	@if ./${PROG} ; then		\
		echo "PASSED";		\
	else				\
		echo "FAILED";		\
	fi

.include <bsd.prog.mk>

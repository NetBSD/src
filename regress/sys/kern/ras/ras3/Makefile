#	$NetBSD: Makefile,v 1.1 2002/08/28 07:47:27 gmcgarry Exp $

PROG=	ras3
NOMAN=	#defined
WARNS=	2

regress:
	@if ./${PROG} ; then		\
		echo "PASSED";		\
	else				\
		echo "FAILED";		\
	fi

.include <bsd.prog.mk>

#	$NetBSD: Makefile,v 1.1 2004/07/06 12:28:20 yamt Exp $

NOMAN=	#defined

PROG=	sa1
WARNS=	3

regress:
	@if ./${PROG} ; then		\
		echo "PASSED";		\
	else				\
		echo "FAILED";		\
	fi

.include <bsd.prog.mk>

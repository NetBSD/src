#	$NetBSD: Makefile,v 1.2 2002/09/18 05:41:53 lukem Exp $

NOMAN=		# defined

PROG=		lock
WARNS=		2

regress:
	@if ./lock ; then		\
		echo "PASSED";		\
	else				\
		echo "FAILED";		\
	fi

.include <bsd.prog.mk>

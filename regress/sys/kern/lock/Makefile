#	$NetBSD: Makefile,v 1.1 2002/08/28 07:41:05 gmcgarry Exp $

PROG=		lock
NOMAN=		# defined
WARNS=		2

regress:
	@if ./lock ; then		\
		echo "PASSED";		\
	else				\
		echo "FAILED";		\
	fi

.include <bsd.prog.mk>

#	$NetBSD: Makefile.inc,v 1.1 2004/09/26 03:45:10 yamt Exp $

NOMAN=		# defined
WARNS=		3

CPPFLAGS+=      -I${.CURDIR}/..

regress:
	@if ./${PROG}; then		\
		echo PASSED; exit 0;	\
	else				\
		echo FAILED; exit 1;	\
	fi

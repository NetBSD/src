#	$NetBSD: Makefile,v 1.2 2001/12/12 01:24:04 tv Exp $

PROG=		clonetest
NOMAN=		# defined
COPTS+=		-g
WARNS?=		1

regress:
	@if ./clonetest; then \
		echo PASSED; \
	else \
		echo FAILED; \
		exit 1; \
	fi

.include <bsd.prog.mk>

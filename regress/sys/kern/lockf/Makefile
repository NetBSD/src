#	$NetBSD: Makefile,v 1.2 2001/12/12 01:24:16 tv Exp $

PROG=		lockf
NOMAN=		# defined
WARNS=		2

regress:
	@if ./lockf ; then \
		echo "PASSED"; \
	else \
		echo "FAILED"; \
	fi

.include <bsd.prog.mk>

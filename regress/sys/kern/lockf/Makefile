#	$NetBSD: Makefile,v 1.3 2002/09/18 05:41:54 lukem Exp $

NOMAN=		# defined

PROG=		lockf
WARNS=		2

regress:
	@if ./lockf ; then \
		echo "PASSED"; \
	else \
		echo "FAILED"; \
	fi

.include <bsd.prog.mk>

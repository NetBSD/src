#	$NetBSD: Makefile,v 1.4 2002/09/18 05:41:55 lukem Exp $

NOMAN=		# defined

PROG=		semtest
WARNS=		1

regress:
	@if ./semtest ./semtest; then \
		echo "PASSED"; \
	else \
		echo "FAILED"; \
	fi

.include <bsd.prog.mk>

#	$NetBSD: Makefile,v 1.3 2002/03/05 21:57:00 thorpej Exp $

PROG=		semtest
NOMAN=		# defined
WARNS=		1

regress:
	@if ./semtest ./semtest; then \
		echo "PASSED"; \
	else \
		echo "FAILED"; \
	fi

.include <bsd.prog.mk>

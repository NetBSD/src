#	$NetBSD: Makefile,v 1.3 2002/03/05 21:56:59 thorpej Exp $

PROG=		msgtest
NOMAN=		# defined
WARNS=		1

regress:
	@if ./msgtest ./msgtest; then \
		echo "PASSED"; \
	else \
		echo "FAILED"; \
	fi

.include <bsd.prog.mk>

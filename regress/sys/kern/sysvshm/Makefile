#	$NetBSD: Makefile,v 1.4 2002/09/18 05:41:56 lukem Exp $

NOMAN=		# defined

PROG=		shmtest
WARNS=		1

regress:
	@if ./shmtest ./shmtest; then \
		echo "PASSED"; \
	else \
		echo "FAILED"; \
	fi

.include <bsd.prog.mk>

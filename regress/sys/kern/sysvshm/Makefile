#	$NetBSD: Makefile,v 1.2 2001/12/12 01:24:17 tv Exp $

PROG=		shmtest
NOMAN=		# defined
WARNS=		1

regress:
	@if ./shmtest ${.OBJDIR}/shmtest; then \
		echo "PASSED"; \
	else \
		echo "FAILED"; \
	fi

.include <bsd.prog.mk>

#	$NetBSD: Makefile,v 1.2 2001/12/12 01:24:17 tv Exp $

PROG=		semtest
NOMAN=		# defined
WARNS=		1

regress:
	@if ./semtest ${.OBJDIR}/semtest; then \
		echo "PASSED"; \
	else \
		echo "FAILED"; \
	fi

.include <bsd.prog.mk>

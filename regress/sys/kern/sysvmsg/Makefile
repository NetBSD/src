#	$NetBSD: Makefile,v 1.2 2001/12/12 01:24:17 tv Exp $

PROG=		msgtest
NOMAN=		# defined
WARNS=		1

regress:
	@if ./msgtest ${.OBJDIR}/msgtest; then \
		echo "PASSED"; \
	else \
		echo "FAILED"; \
	fi

.include <bsd.prog.mk>

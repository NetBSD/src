#	$NetBSD: Makefile,v 1.1 2004/02/28 16:55:05 drochner Exp $

NOMAN=	# defined

PROG=	context

regress: ${PROG}
	@if ./${PROG}; then \
		echo "PASSED"; \
	else \
		echo "FAILED"; \
	fi

.include <bsd.prog.mk>

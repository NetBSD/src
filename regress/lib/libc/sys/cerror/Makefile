#	$NetBSD: Makefile,v 1.1 2003/02/07 21:00:44 cgd Exp $

NOMAN=		# defined

PROG=		cerror
WARNS=		2

regress: ${PROG}
	@if ./${PROG} > /dev/null; then \
		echo "PASSED"; \
	else \
		echo "FAILED"; \
	fi

.include <bsd.prog.mk>

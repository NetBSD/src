#	$NetBSD: Makefile,v 1.1 2005/03/27 18:49:51 christos Exp $

NOMAN=		# defined

PROG=		mbtowc_test
WARNS?=		1

regress: ${PROG}
	@if ./${PROG}; then \
		echo PASSED; \
	else \
		echo FAILED; \
		exit 1; \
	fi

.include <bsd.prog.mk>

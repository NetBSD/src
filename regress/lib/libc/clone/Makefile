#	$NetBSD: Makefile,v 1.3 2002/04/21 20:23:59 bjh21 Exp $

PROG=		clonetest
NOMAN=		# defined
COPTS+=		-g
WARNS?=		1

regress: ${PROG}
	@if ./clonetest; then \
		echo PASSED; \
	else \
		echo FAILED; \
		exit 1; \
	fi

.include <bsd.prog.mk>

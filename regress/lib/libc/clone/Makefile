#	$NetBSD: Makefile,v 1.4 2002/09/18 05:41:37 lukem Exp $

NOMAN=		# defined

PROG=		clonetest
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

#	$NetBSD: Makefile,v 1.1 2005/03/04 21:42:40 dsl Exp $

NOMAN=		# defined

PROG=		strptimetest
COPTS+=		-g
WARNS?=		1

regress: ${PROG}
	@if ./strptimetest; then \
		echo PASSED; \
	else \
		echo FAILED; \
		exit 1; \
	fi

.include <bsd.prog.mk>

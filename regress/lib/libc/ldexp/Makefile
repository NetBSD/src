#	$NetBSD: Makefile,v 1.3 2001/12/12 01:24:02 tv Exp $

PROG=		ldexp
NOMAN=		# defined
CLEANFILES+=	output

regress: ${PROG}
	@./${PROG} >output
	@if ! cmp -s ${.CURDIR}/expected output; then \
		echo "FAILED"; \
	else \
		echo "PASSED"; \
	fi

.include <bsd.prog.mk>

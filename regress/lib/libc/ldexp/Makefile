#	$NetBSD: Makefile,v 1.4 2002/09/18 05:41:39 lukem Exp $

NOMAN=		# defined

PROG=		ldexp
CLEANFILES+=	output

regress: ${PROG}
	@./${PROG} >output
	@if ! cmp -s ${.CURDIR}/expected output; then \
		echo "FAILED"; \
	else \
		echo "PASSED"; \
	fi

.include <bsd.prog.mk>

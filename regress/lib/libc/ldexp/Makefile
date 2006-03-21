#	$NetBSD: Makefile,v 1.5 2006/03/21 18:34:22 drochner Exp $

NOMAN=		# defined

PROG=		ldexp
LDADD+=		-lm
CLEANFILES+=	output

regress: ${PROG}
	@./${PROG} >output
	@if ! cmp -s ${.CURDIR}/expected output; then \
		echo "FAILED"; \
	else \
		echo "PASSED"; \
	fi

.include <bsd.prog.mk>

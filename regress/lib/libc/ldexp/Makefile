#	$NetBSD: Makefile,v 1.1 1999/08/30 17:03:17 mycroft Exp $

PROG=	ldexp
MKMAN=	no

regress: ${PROG}
	@./${PROG} >output
	@if ! cmp -s ${.CURDIR}/expected output; then \
		echo "FAILED"; \
	else \
		echo "PASSED"; \
	fi

.include <bsd.prog.mk>

#	$NetBSD: Makefile,v 1.1 1998/01/07 03:53:00 thorpej Exp $

PROG=	unfdpass
NOMAN=	noman, no way, man

CLEANFILES+=file1 file2 output

regress:
	@./unfdpass > output
	@if ! cmp -s ${.CURDIR}/expected output; then \
		echo "FAILED"; \
	else \
		echo "PASSED"; \
	fi

.include <bsd.prog.mk>

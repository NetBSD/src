#	$NetBSD: Makefile,v 1.2 1998/01/07 23:38:53 thorpej Exp $

PROG=	unfdpass
NOMAN=	noman, no way, man
LDSTATIC=-static

CLEANFILES+=file1 file2 output test-sock

regress:
	@./unfdpass > output
	@if ! cmp -s ${.CURDIR}/expected output; then \
		echo "FAILED"; \
	else \
		echo "PASSED"; \
	fi

.include <bsd.prog.mk>

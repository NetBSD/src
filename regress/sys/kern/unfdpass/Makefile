#	$NetBSD: Makefile,v 1.4 1999/02/13 02:54:33 lukem Exp $

PROG=	unfdpass
MKMAN=	no

CLEANFILES+=file1 file2 output test-sock

regress:
	@./unfdpass > output
	@if ! cmp -s ${.CURDIR}/expected output; then \
		echo "FAILED"; \
	else \
		echo "PASSED"; \
	fi

.include <bsd.prog.mk>

#	$NetBSD: Makefile,v 1.6 2000/06/05 06:01:42 thorpej Exp $

PROG=	unfdpass
MKMAN=	no

WARNS?=	1

CLEANFILES+=file1 file2 file3 file4 file5 file6 file7 file8 file9 file10 \
	file11 file12 file13 file14 file15 file16 file17 file18 file19 \
	file20 file21 file22 file23 file24 output test-sock

regress:
	@./unfdpass > output
	@if ! cmp -s ${.CURDIR}/expected output; then \
		echo "FAILED"; \
	else \
		echo "PASSED"; \
	fi

.include <bsd.prog.mk>

#	$NetBSD: Makefile,v 1.1 1999/06/19 00:53:32 thorpej Exp $

PROG=	mmap
MKMAN=	no

# This is hopefully just some file not mmap'd very often.
SOME_FILE=/usr/share/misc/zipcodes

regress:
	@if ./mmap ${SOME_FILE}; then \
		echo "PASSED"; \
	else \
		echo "FAILED"; \
	fi

.include <bsd.prog.mk>

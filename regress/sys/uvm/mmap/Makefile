#	$NetBSD: Makefile,v 1.2 1999/07/14 21:10:13 thorpej Exp $

PROG=	mmap
MKMAN=	no

CLEANFILES+= mapped_file

# This is hopefully just some file not mmap'd very often.
SOME_FILE=/usr/share/misc/zipcodes

regress:
	@if ./mmap ${SOME_FILE}; then \
		echo "PASSED"; \
	else \
		echo "FAILED"; \
	fi

.include <bsd.prog.mk>

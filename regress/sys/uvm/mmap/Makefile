#	$NetBSD: Makefile,v 1.3 2001/12/12 01:24:18 tv Exp $

PROG=		mmap
NOMAN=		# defined
CLEANFILES+=	mapped_file

# This is hopefully just some file not mmap'd very often.
SOME_FILE=	/usr/share/misc/zipcodes

regress:
	@if ./mmap ${SOME_FILE}; then \
		echo "PASSED"; \
	else \
		echo "FAILED"; \
	fi

.include <bsd.prog.mk>

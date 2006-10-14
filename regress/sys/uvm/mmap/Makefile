#	$NetBSD: Makefile,v 1.5 2006/10/14 20:57:10 bjh21 Exp $

NOMAN=		# defined

PROG=		mmap
CLEANFILES+=	mapped_file

# This is hopefully just some file not mmap'd very often.
SOME_FILE=	/usr/share/misc/zipcodes

regress: ${PROG}
	@if ./mmap ${SOME_FILE}; then \
		echo "PASSED"; \
	else \
		echo "FAILED"; \
	fi

.include <bsd.prog.mk>

#	$NetBSD: Makefile,v 1.4 2002/09/18 05:41:57 lukem Exp $

NOMAN=		# defined

PROG=		mmap
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

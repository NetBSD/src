#	$NetBSD: Makefile,v 1.1 2001/07/17 23:58:30 thorpej Exp $

MKMAN=no

PROG=	clonetest
COPTS+=-g

WARNS?=	1

regress:
	@if ./clonetest; then \
		echo PASSED; \
	else \
		echo FAILED; \
		exit 1; \
	fi

.include <bsd.prog.mk>

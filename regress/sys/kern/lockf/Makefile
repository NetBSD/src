#	$NetBSD: Makefile,v 1.1 2000/06/12 14:42:03 sommerfeld Exp $

PROG=	lockf
MKMAN=	no
WARNS=	2

regress:
	@if ./lockf ; then \
		echo "PASSED"; \
	else \
		echo "FAILED"; \
	fi

.include <bsd.prog.mk>

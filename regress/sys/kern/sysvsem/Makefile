#	$NetBSD: Makefile,v 1.1 1999/08/25 04:35:34 thorpej Exp $

PROG=	semtest
MKMAN=	no
WARNS=	1

regress:
	@if ./semtest ${.OBJDIR}/semtest; then \
		echo "PASSED"; \
	else \
		echo "FAILED"; \
	fi

.include <bsd.prog.mk>

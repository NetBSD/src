#	$NetBSD: Makefile,v 1.1 1999/08/24 22:08:56 thorpej Exp $

PROG=	msgtest
MKMAN=	no
WARNS=	1

regress:
	@if ./msgtest ${.OBJDIR}/msgtest; then \
		echo "PASSED"; \
	else \
		echo "FAILED"; \
	fi

.include <bsd.prog.mk>

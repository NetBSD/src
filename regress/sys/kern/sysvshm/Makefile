#	$NetBSD: Makefile,v 1.1 1999/08/24 23:01:56 thorpej Exp $

PROG=	shmtest
MKMAN=	no
WARNS=	1

regress:
	@if ./shmtest ${.OBJDIR}/shmtest; then \
		echo "PASSED"; \
	else \
		echo "FAILED"; \
	fi

.include <bsd.prog.mk>

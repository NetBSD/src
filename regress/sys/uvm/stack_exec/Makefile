# $NetBSD: Makefile,v 1.1 2003/12/10 13:24:59 drochner Exp $

NOMAN=	#
PROG=	tramptest
LDADD=	-lpthread

regress:
	@if ./tramptest; then \
		echo "PASSED"; \
	else \
		echo "FAILED"; \
	fi

.include <bsd.prog.mk>

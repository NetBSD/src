# $NetBSD: Makefile,v 1.2 2006/10/14 20:57:10 bjh21 Exp $

NOMAN=	#
PROG=	tramptest
LDADD=	-lpthread

regress: ${PROG}
	@if ./tramptest; then \
		echo "PASSED"; \
	else \
		echo "FAILED"; \
	fi

.include <bsd.prog.mk>

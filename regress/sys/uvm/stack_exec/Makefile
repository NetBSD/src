# $NetBSD: Makefile,v 1.3 2011/05/20 13:36:12 joerg Exp $

NOMAN=	#
PROG=	tramptest
LDADD=	-lpthread

UNSUPPORTED_COMPILER.clang=
UNSUPPORTED_COMPILER.pcc=

regress: ${PROG}
	@if ./tramptest; then \
		echo "PASSED"; \
	else \
		echo "FAILED"; \
	fi

.include <bsd.prog.mk>

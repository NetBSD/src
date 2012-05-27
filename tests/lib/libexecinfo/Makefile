# $NetBSD: Makefile,v 1.1 2012/05/27 18:47:18 christos Exp $

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/lib/libm

TESTS_C+=	t_backtrace

LDADD+=		-lexecinfo -lelf
DPADD+=		${LIBEXECINFO} ${LIBELF}
DBG=-O0		# prevent inlining, nothing else helps.

.include <bsd.test.mk>

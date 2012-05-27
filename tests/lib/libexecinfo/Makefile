# $NetBSD: Makefile,v 1.2 2012/05/27 22:57:24 martin Exp $

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/lib/libexecinfo

TESTS_C+=	t_backtrace

LDADD+=		-lexecinfo -lelf
DPADD+=		${LIBEXECINFO} ${LIBELF}
DBG=-O0		# prevent inlining, nothing else helps.

.include <bsd.test.mk>

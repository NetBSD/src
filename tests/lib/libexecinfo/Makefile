# $NetBSD: Makefile,v 1.2.4.2 2012/10/30 19:00:03 yamt Exp $

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/lib/libexecinfo

TESTS_C+=	t_backtrace

LDADD+=		-lexecinfo -lelf
DPADD+=		${LIBEXECINFO} ${LIBELF}
DBG=-O0		# prevent inlining, nothing else helps.

.include <bsd.test.mk>

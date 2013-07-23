# $NetBSD: Makefile,v 1.3.2.1 2013/07/23 21:07:38 riastradh Exp $

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/lib/libexecinfo

TESTS_C+=	t_backtrace

LDADD+=		-lexecinfo -lelf
DPADD+=		${LIBEXECINFO} ${LIBELF}

COPTS.t_backtrace.c=	-Wno-stack-protector

.include <bsd.test.mk>

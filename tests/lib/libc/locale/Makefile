# $NetBSD: Makefile,v 1.2 2011/04/11 17:16:43 tron Exp $

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/lib/libc/locale

TESTS_C+=	t_ctype1
TESTS_C+=	t_ctype2
TESTS_C+=	t_mbtowc
TESTS_C+=	t_wcstod
TESTS_C+=	t_wctomb

COPTS.t_wctomb.c += -Wno-stack-protector

.include <bsd.test.mk>

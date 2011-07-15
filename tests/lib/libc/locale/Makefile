# $NetBSD: Makefile,v 1.3 2011/07/15 07:35:21 jruoho Exp $

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/lib/libc/locale

TESTS_C+=	t_mbrtowc
TESTS_C+=	t_mbstowcs
TESTS_C+=	t_mbtowc
TESTS_C+=	t_wcstod
TESTS_C+=	t_wctomb

COPTS.t_wctomb.c += -Wno-stack-protector

.include <bsd.test.mk>

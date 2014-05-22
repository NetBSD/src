# $NetBSD: Makefile,v 1.3.2.2 2014/05/22 11:42:20 yamt Exp $

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/lib/libc/locale

TESTS_C+=	t_mbrtowc
TESTS_C+=	t_mbstowcs
TESTS_C+=	t_mbsnrtowcs
TESTS_C+=	t_mbtowc
TESTS_C+=	t_wcscspn
TESTS_C+=	t_wcspbrk
TESTS_C+=	t_wcsspn
TESTS_C+=	t_wcstod
TESTS_C+=	t_wctomb
TESTS_C+=	t_io

COPTS.t_wctomb.c += -Wno-stack-protector

.include <bsd.test.mk>

# $NetBSD: Makefile,v 1.7 2017/05/30 02:11:03 perseant Exp $

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
TESTS_C+=	t_toupper

COPTS.t_wctomb.c += -Wno-stack-protector

.include <bsd.test.mk>

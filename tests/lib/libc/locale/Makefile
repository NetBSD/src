# $NetBSD: Makefile,v 1.8 2017/05/30 23:44:02 perseant Exp $

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
#TESTS_C+=	t_digittoint
TESTS_C+=	t_sprintf
TESTS_C+=	t_wctype

COPTS.t_wctomb.c += -Wno-stack-protector

.include <bsd.test.mk>

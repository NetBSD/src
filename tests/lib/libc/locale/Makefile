# $NetBSD: Makefile,v 1.4.6.1 2013/06/23 06:28:56 tls Exp $

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

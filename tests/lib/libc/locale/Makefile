# $NetBSD: Makefile,v 1.12 2017/08/16 13:53:20 joerg Exp $

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
TESTS_C+=	t_digittoint
TESTS_C+=	t_sprintf
TESTS_C+=	t_wctype
TESTS_C+=	t_btowc
TESTS_C+=	t_wcscoll
TESTS_C+=	t_ducet
TESTS_C+=	t_strfmon

COPTS.t_wctomb.c += -Wno-stack-protector
COPTS.t_digittoint.c += -Wno-unused-variable
COPTS.t_btowc.c += -Wno-unused-variable

.include <bsd.test.mk>

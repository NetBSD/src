# $NetBSD: Makefile,v 1.13.10.1 2024/10/14 17:20:19 martin Exp $

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/lib/libc/locale

TESTS_C+=	t_btowc
TESTS_C+=	t_c16rtomb
TESTS_C+=	t_c32rtomb
TESTS_C+=	t_c8rtomb
TESTS_C+=	t_digittoint
TESTS_C+=	t_ducet
TESTS_C+=	t_io
TESTS_C+=	t_mbrtoc16
TESTS_C+=	t_mbrtoc32
TESTS_C+=	t_mbrtoc8
TESTS_C+=	t_mbrtowc
TESTS_C+=	t_mbsnrtowcs
TESTS_C+=	t_mbstowcs
TESTS_C+=	t_mbtowc
TESTS_C+=	t_sprintf
TESTS_C+=	t_strfmon
TESTS_C+=	t_toupper
TESTS_C+=	t_uchar
TESTS_C+=	t_wcscoll
TESTS_C+=	t_wcscspn
TESTS_C+=	t_wcspbrk
TESTS_C+=	t_wcsrtombs
TESTS_C+=	t_wcsspn
TESTS_C+=	t_wcstod
TESTS_C+=	t_wctomb
TESTS_C+=	t_wctype

COPTS.t_btowc.c += -Wno-unused-variable
COPTS.t_digittoint.c += -Wno-unused-variable
COPTS.t_wctomb.c += -Wno-stack-protector

.include <bsd.test.mk>

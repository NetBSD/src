# $NetBSD: Makefile,v 1.14 2023/06/03 09:09:15 lukem Exp $

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/lib/libc/string
DBG=-g

COPTS.t_strcat.c+= ${CC_WNO_STRINGOP_TRUNCATION}

TESTS_C+=	t_bm
TESTS_C+=	t_memchr
TESTS_C+=	t_memcpy
TESTS_C+=	t_memmem
TESTS_C+=	t_memset
TESTS_C+=	t_popcount
TESTS_C+=	t_strcat
TESTS_C+=	t_strchr
TESTS_C+=	t_strchrnul
TESTS_C+=	t_strcmp
TESTS_C+=	t_strcoll
TESTS_C+=	t_strcpy
TESTS_C+=	t_strcspn
TESTS_C+=	t_strerror
TESTS_C+=	t_stresep
TESTS_C+=	t_strlen
TESTS_C+=	t_strpbrk
TESTS_C+=	t_strrchr
TESTS_C+=	t_strspn
TESTS_C+=	t_swab

WARNS=		4

.include <bsd.test.mk>

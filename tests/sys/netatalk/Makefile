# $NetBSD: Makefile,v 1.2 2021/04/12 02:54:08 mrg Exp $
#
WARNS?=6

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/sys/netatalk
CPPFLAGS+=	-I${NETBSDSRCDIR}/sys -D_TEST

TESTS_C=	t_print

COPTS.t_print.c+=	${GCC_NO_FORMAT_TRUNCATION}

.include <bsd.test.mk>

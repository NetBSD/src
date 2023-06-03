# $NetBSD: Makefile,v 1.3 2023/06/03 09:09:16 lukem Exp $
#
WARNS?=6

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/sys/netatalk
CPPFLAGS+=	-I${NETBSDSRCDIR}/sys -D_TEST

TESTS_C=	t_print

COPTS.t_print.c+=	${CC_WNO_FORMAT_TRUNCATION}

.include <bsd.test.mk>

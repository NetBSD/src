# $NetBSD: Makefile,v 1.1 2016/11/14 16:10:31 pho Exp $

NOMAN=		# defined

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/lib/librefuse

DPADD+=		${LIBREFUSE}
LDADD+=		-lrefuse

TESTS_C=	t_refuse_opt

.include <bsd.test.mk>

# $NetBSD: Makefile,v 1.1.2.2 2017/01/07 08:56:55 pgoyette Exp $

NOMAN=		# defined

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/lib/librefuse

DPADD+=		${LIBREFUSE}
LDADD+=		-lrefuse

TESTS_C=	t_refuse_opt

.include <bsd.test.mk>

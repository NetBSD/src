# $NetBSD: Makefile,v 1.1.4.2 2012/04/17 00:09:13 yamt Exp $

NOMAN=		# defined 

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/lib/libcrypt

LDADD+=		-lcrypt
DPADD+=		${LIBCRYPT}
WARNS?=		4

TESTS_C=	t_crypt

.include <bsd.test.mk>

# $NetBSD: Makefile,v 1.2 2021/10/12 10:52:40 nia Exp $

NOMAN=		# defined 

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/lib/libcrypt

LDADD+=		-lcrypt
DPADD+=		${LIBCRYPT}
WARNS?=		4

TESTS_C=	t_crypt

.if defined(MKARGON2) && ${MKARGON2} != "no"
CPPFLAGS+=	-DHAVE_ARGON2
.endif

.include <bsd.test.mk>

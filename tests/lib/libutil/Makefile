# $NetBSD: Makefile,v 1.2 2010/12/20 16:23:01 njoly Exp $

NOMAN=		# defined

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/lib/libutil

DPADD+=		${LIBUTIL}
LDADD+=		-lutil

TESTS_C=	t_parsedate
TESTS_C+=	t_snprintb
TESTS_C+=	t_sockaddr_snprintf

.if (${USE_INET6} != "no")
CFLAGS+=	-DINET6
.endif

.include <bsd.test.mk>

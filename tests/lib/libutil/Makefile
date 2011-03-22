# $NetBSD: Makefile,v 1.3 2011/03/22 23:07:32 jmmv Exp $

NOMAN=		# defined

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/lib/libutil

DPADD+=		${LIBUTIL}
LDADD+=		-lutil

TESTS_C=	t_parsedate
TESTS_C+=	t_pidfile
TESTS_C+=	t_snprintb
TESTS_C+=	t_sockaddr_snprintf

.if (${USE_INET6} != "no")
CFLAGS+=	-DINET6
.endif

.include <bsd.test.mk>

# $NetBSD: Makefile,v 1.4.56.1 2025/08/02 05:58:09 perseant Exp $

NOMAN=		# defined

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/lib/libutil

DPADD+=		${LIBUTIL}
LDADD+=		-lutil

TESTS_C=	t_efun
TESTS_C+=	t_parsedate
TESTS_C+=	t_pidfile
TESTS_C+=	t_snprintb
TESTS_C+=	t_sockaddr_snprintf
TESTS_C+=	t_strpct

.if (${USE_INET6} != "no")
CFLAGS+=	-DINET6
.endif

.include <bsd.test.mk>

# $NetBSD: Makefile,v 1.5 2025/05/02 19:52:02 rillig Exp $

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

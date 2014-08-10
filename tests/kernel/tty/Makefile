# $NetBSD: Makefile,v 1.1.22.1 2014/08/10 06:57:19 tls Exp $

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/kernel/tty

TESTS_C=	t_pr

LDADD+=		-lrumpkern_tty -lrumpvfs -lrump -lrumpuser
LDADD+=		-lrump
LDADD+=		-lpthread

.include <bsd.test.mk>

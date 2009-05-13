# $NetBSD: Makefile,v 1.1.2.2 2009/05/13 19:19:34 jym Exp $

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/rump/rumpkern

TESTS_C=	t_modcmd

LDADD+=		-lrumpvfs -lrump -lrumpuser -lpthread

.include <bsd.test.mk>

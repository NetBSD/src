# $NetBSD: Makefile,v 1.1 2009/05/02 16:02:19 pooka Exp $

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/rump/rumpkern

TESTS_C=	t_modcmd

LDADD+=		-lrumpvfs -lrump -lrumpuser -lpthread

.include <bsd.test.mk>

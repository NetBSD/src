# $NetBSD: Makefile,v 1.2 2009/06/09 11:18:50 he Exp $

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/rump/rumpkern

TESTS_C=	t_modcmd

LDADD+=		-lrump -lrumpvfs -lrump -lrumpuser -lpthread

.include <bsd.test.mk>

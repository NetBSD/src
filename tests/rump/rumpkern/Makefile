# $NetBSD: Makefile,v 1.3 2009/09/24 21:36:42 pooka Exp $

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/rump/rumpkern

TESTS_C=	t_modcmd
TESTS_C+=	t_modlinkset

LDADD.t_modlinkset+=	-lukfs -lrumpfs_cd9660 -lrumpfs_msdos
LDADD+=			-lrumpvfs -lrump -lrumpuser -lpthread

.include <bsd.test.mk>

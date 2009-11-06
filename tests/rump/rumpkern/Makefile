# $NetBSD: Makefile,v 1.4 2009/11/06 15:26:54 pooka Exp $

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/rump/rumpkern

TESTS_C=	t_modcmd
TESTS_C+=	t_modlinkset

LDADD.t_modlinkset+=	-lukfs -lrumpfs_cd9660 -lrumpfs_msdos
LDADD+=			-lrumpvfs -lrump -lrumpuser -lpthread

WARNS=	4

.include <bsd.test.mk>

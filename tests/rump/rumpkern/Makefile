# $NetBSD: Makefile,v 1.5 2009/11/28 22:20:38 he Exp $

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/rump/rumpkern

TESTS_C=	t_modcmd
TESTS_C+=	t_modlinkset

ADD_TO_LD=	-lrumpvfs -lrump -lrumpuser -lpthread
LDADD.t_modlinkset+=	-lukfs -lrumpfs_cd9660 -lrumpfs_msdos ${ADD_TO_LD}
LDADD+=			${ADD_TO_LD}

WARNS=	4

.include <bsd.test.mk>

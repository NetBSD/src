# $NetBSD: Makefile,v 1.6 2010/01/25 19:27:24 pooka Exp $

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/rump/rumpkern

TESTS_C=	t_modcmd
TESTS_C+=	t_modlinkset

ADD_TO_LD=	-lrumpvfs -lrump -lrumpuser -lpthread
LDADD.t_modlinkset+=	-lukfs -lrumpdev_disk -lrumpdev -lrumpfs_msdos 
LDADD.t_modlinkset+=	-lrumpfs_cd9660 ${ADD_TO_LD}
LDADD+=			${ADD_TO_LD}

WARNS=	4

.include <bsd.test.mk>

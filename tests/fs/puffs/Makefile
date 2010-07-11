#	$NetBSD: Makefile,v 1.7 2010/07/11 12:33:38 pooka Exp $
#

.include <bsd.own.mk>

SUBDIR+=	h_dtfs

TESTSDIR=	${TESTSBASE}/fs/puffs

TESTS_C=	t_basic

LDADD+=		-lrumpdev_putter -lrumpdev
LDADD+=		-lrumpfs_syspuffs -lrumpvfs_fifofs
LDADD+=		-lrumpnet_local -lrumpnet_net -lrumpnet
LDADD+=		-lrumpvfs
LDADD+=		-lrump -lrumpuser -lpthread
LDADD+=		-lpuffs -lutil

WARNS=2

.include <bsd.test.mk>
.include <bsd.subdir.mk>

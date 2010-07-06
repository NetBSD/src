#	$NetBSD: Makefile,v 1.4 2010/07/06 14:44:30 pooka Exp $
#

.include <bsd.own.mk>

SUBDIR+=	h_dtfs

TESTSDIR=	${TESTSBASE}/fs/puffs

TESTS_C=	t_basic

LDADD+=		-lrumpdev_putter -lrumpdev
LDADD+=		-lrumpfs_syspuffs -lrumpvfs -lrump -lrumpuser -lpthread

.include <bsd.test.mk>
.include <bsd.subdir.mk>

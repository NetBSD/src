#	$NetBSD: Makefile,v 1.9 2010/07/13 21:13:23 jmmv Exp $
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

# required by -DPUFFSDUMP
#LDADD+= 	-lpuffs -lutil

WARNS=2

.include <bsd.test.mk>

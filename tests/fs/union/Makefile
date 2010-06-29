#	$NetBSD: Makefile,v 1.2 2010/06/29 15:25:28 pooka Exp $
#

TESTSDIR=	${TESTSBASE}/fs/union
WARNS=		4

TESTS_C=	t_basic t_pr

LDADD+=	-lrumpfs_union -lrumpvfs_layerfs -lrumpfs_ffs		# fs drivers
LDADD+=	-lrumpdev_disk -lrumpdev				# disk device
LDADD+=	-lrumpvfs -lrump -lrumpuser -lpthread			# base

.include <bsd.test.mk>

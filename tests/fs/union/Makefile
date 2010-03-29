#	$NetBSD: Makefile,v 1.1 2010/03/29 18:19:19 pooka Exp $
#

TESTSDIR=	${TESTSBASE}/fs/union
WARNS=		4

TESTS_C=	t_basic

LDADD+=	-lrumpfs_union -lrumpvfs_layerfs -lrumpfs_ffs		# fs drivers
LDADD+=	-lrumpdev_disk -lrumpdev				# disk device
LDADD+=	-lrumpvfs -lrump -lrumpuser -lpthread			# base

.include <bsd.test.mk>

#	$NetBSD: Makefile,v 1.4.24.1 2020/04/08 14:09:07 martin Exp $
#

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/fs/union
WARNS=		4

TESTS_C=	t_pr

LDADD+=	-lrumpfs_union -lrumpvfs_layerfs -lrumpfs_ffs		# fs drivers
LDADD+=	-lrumpdev_disk -lrumpdev				# disk device
LDADD+=	${LIBRUMPBASE}						# base

.include <bsd.test.mk>

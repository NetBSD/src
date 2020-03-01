#	$NetBSD: Makefile,v 1.5 2020/03/01 18:08:14 christos Exp $
#

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/fs/nullfs
WARNS=		4

TESTS_C=	t_basic

LDADD+=	-lrumpfs_tmpfs -lrumpfs_null -lrumpvfs_layerfs		# fs drivers
LDADD+=	${LIBRUMPBASE}						# base

.include <bsd.test.mk>

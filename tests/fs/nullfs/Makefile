#	$NetBSD: Makefile,v 1.4.24.1 2020/04/08 14:09:07 martin Exp $
#

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/fs/nullfs
WARNS=		4

TESTS_C=	t_basic

LDADD+=	-lrumpfs_tmpfs -lrumpfs_null -lrumpvfs_layerfs		# fs drivers
LDADD+=	${LIBRUMPBASE}						# base

.include <bsd.test.mk>

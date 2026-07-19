#	$NetBSD$

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/fs/fullfs
WARNS=		4

TESTS_C=	t_basic

LDADD+=	-lrumpfs_tmpfs -lrumpfs_full -lrumpvfs_layerfs		# fs drivers
LDADD+=	${LIBRUMPBASE}						# base

.include <bsd.test.mk>

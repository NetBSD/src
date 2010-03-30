#	$NetBSD: Makefile,v 1.1 2010/03/30 01:02:47 pooka Exp $
#

TESTSDIR=	${TESTSBASE}/fs/union
WARNS=		4

TESTS_C=	t_basic

LDADD+=	-lrumpfs_tmpfs -lrumpfs_nullfs -lrumpvfs_layerfs	# fs drivers
LDADD+=	-lrumpvfs -lrump -lrumpuser -lpthread			# base

.include <bsd.test.mk>

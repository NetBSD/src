#	$NetBSD: Makefile,v 1.2 2010/03/30 01:40:29 pooka Exp $
#

TESTSDIR=	${TESTSBASE}/fs/nullfs
WARNS=		4

TESTS_C=	t_basic

LDADD+=	-lrumpfs_tmpfs -lrumpfs_nullfs -lrumpvfs_layerfs	# fs drivers
LDADD+=	-lrumpvfs -lrump -lrumpuser -lpthread			# base

.include <bsd.test.mk>

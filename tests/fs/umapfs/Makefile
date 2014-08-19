#	$NetBSD: Makefile,v 1.3.12.1 2014/08/20 00:04:47 tls Exp $
#

TESTSDIR=	${TESTSBASE}/fs/umapfs
WARNS=		4

TESTS_C=	t_basic

LDADD+=	-lrumpfs_tmpfs -lrumpfs_umap -lrumpvfs_layerfs		# fs drivers
LDADD+=	-lrumpvfs -lrump -lrumpuser -lrump -lpthread		# base

.include <bsd.test.mk>

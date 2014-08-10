#	$NetBSD: Makefile,v 1.3.22.1 2014/08/10 06:57:07 tls Exp $
#

TESTSDIR=	${TESTSBASE}/fs/nullfs
WARNS=		4

TESTS_C=	t_basic

LDADD+=	-lrumpfs_tmpfs -lrumpfs_null -lrumpvfs_layerfs		# fs drivers
LDADD+=	-lrumpvfs -lrump -lrumpuser -lrump -lpthread		# base

.include <bsd.test.mk>

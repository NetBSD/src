#	$NetBSD: Makefile,v 1.4 2009/05/02 01:15:54 pooka Exp $
#

TESTSDIR=	${TESTSBASE}/fs/ffs
WARNS=		4

TESTS_C=	t_renamerace

LDADD+=	-lrumpfs_ffs -lrumpvfs -lrump -lrumpuser -lpthread

.include <bsd.test.mk>

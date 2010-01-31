#	$NetBSD: Makefile,v 1.5 2010/01/31 14:30:22 mlelstv Exp $
#

TESTSDIR=	${TESTSBASE}/fs/ffs
WARNS=		4

TESTS_C=	t_renamerace

LDADD+=	-lrumpfs_ffs -lrumpdev_disk -lrumpdev -lrumpvfs -lrump -lrumpuser -lpthread

.include <bsd.test.mk>

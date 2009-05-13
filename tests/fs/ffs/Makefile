#	$NetBSD: Makefile,v 1.4.2.2 2009/05/13 19:19:22 jym Exp $
#

TESTSDIR=	${TESTSBASE}/fs/ffs

TESTS_C=	t_renamerace

LDADD.t_renamerace+=	-lukfs -lrumpfs_ffs -lrumpfs_ufs -lrumpvfs -lrump
LDADD.t_renamerace+=	-lrumpuser -lpthread

.include <bsd.test.mk>

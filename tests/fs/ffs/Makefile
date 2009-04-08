#	$NetBSD: Makefile,v 1.1 2009/04/08 12:08:17 pooka Exp $
#

TESTSDIR=	${TESTSBASE}/fs/ffs

TESTS_C=	t_renamerace

LDADD.t_renamerace+=	-lukfs -lrumpfs_ffs -lrumpfs_ufs -lrumpvfs -lrump
LDADD.t_renamerace+=	-lrumpuser -lpthread

.include <bsd.test.mk>

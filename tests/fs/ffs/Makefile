#	$NetBSD: Makefile,v 1.3 2009/04/26 15:15:38 pooka Exp $
#

TESTSDIR=	${TESTSBASE}/fs/ffs
WARNS=		4

TESTS_C=	t_renamerace

LDADD.t_renamerace+=	-lrumpfs_ffs -lrumpfs_ufs -lrumpvfs -lrump
LDADD.t_renamerace+=	-lrumpuser -lpthread

.include <bsd.test.mk>

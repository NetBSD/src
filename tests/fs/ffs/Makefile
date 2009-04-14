#	$NetBSD: Makefile,v 1.2 2009/04/14 10:20:22 pooka Exp $
#

TESTSDIR=	${TESTSBASE}/fs/ffs
WARNS=		4

TESTS_C=	t_renamerace

LDADD.t_renamerace+=	-lukfs -lrumpfs_ffs -lrumpfs_ufs -lrumpvfs -lrump
LDADD.t_renamerace+=	-lrumpuser -lpthread

.include <bsd.test.mk>

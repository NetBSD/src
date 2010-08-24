#	$NetBSD: Makefile,v 1.1 2010/08/24 11:29:45 pooka Exp $
#

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/dev/scsipi

TESTS_C=	t_cd

LDADD+=	-lrumpdev_scsipi -lrumpdev_scsitest -lrumpdev_disk -lrumpdev -lrumpvfs
LDADD+=	-lrump
LDADD+=	-lrumpuser -lpthread

WARNS=	4

.include <bsd.test.mk>

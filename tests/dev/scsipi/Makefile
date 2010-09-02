#	$NetBSD: Makefile,v 1.2 2010/09/02 16:55:15 he Exp $
#

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/dev/scsipi

TESTS_C=	t_cd

LDADD+=	-lrumpdev_scsitest -lrumpdev_scsipi -lrumpdev_disk -lrumpdev -lrumpvfs
LDADD+=	-lrump
LDADD+=	-lrumpuser -lpthread

WARNS=	4

.include <bsd.test.mk>

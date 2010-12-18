#	$NetBSD: Makefile,v 1.3 2010/12/18 15:02:06 christos Exp $
#

TESTSDIR=	${TESTSBASE}/fs/ptyfs
WARNS?=		5

TESTS_C=	t_ptyfs
TESTS_C+=	t_nullpts

LDADD+=	-lrumpfs_ptyfs						# fs drivers
LDADD+=	-lrumpkern_tty						# tty support
LDADD+=	-lrumpvfs -lrump -lrumpuser -lpthread			# base

LDADD.t_nullpts+= -lrumpfs_null -lrumpvfs_layerfs

.include <bsd.test.mk>

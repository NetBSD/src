#	$NetBSD: Makefile,v 1.8 2010/06/30 21:54:56 njoly Exp $
#

TESTSDIR=	${TESTSBASE}/fs/ffs
WARNS=		4

TESTS_C=	t_renamerace
TESTS_C+=	t_fifos
TESTS_C+=	t_snapshot
TESTS_C+=	t_mount

LDADD+=-lrumpfs_ffs						# ffs
LDADD+=-lrumpdev_fss						# snapshot dev
LDADD+=-lrumpdev_disk -lrumpdev					# disk device
LDADD+=-lrumpvfs_fifofs -lrumpnet_local -lrumpnet_net -lrumpnet	# fifos
LDADD+=-lrumpvfs -lrump -lrumpuser -lpthread			# base

.include <bsd.test.mk>

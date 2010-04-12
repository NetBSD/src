#	$NetBSD: Makefile,v 1.7 2010/04/12 22:58:53 pooka Exp $
#

TESTSDIR=	${TESTSBASE}/fs/ffs
WARNS=		4

TESTS_C=	t_renamerace
TESTS_C+=	t_fifos
TESTS_C+=	t_snapshot

LDADD+=-lrumpfs_ffs						# ffs
LDADD+=-lrumpdev_fss						# snapshot dev
LDADD+=-lrumpdev_disk -lrumpdev					# disk device
LDADD+=-lrumpvfs_fifofs -lrumpnet_local -lrumpnet_net -lrumpnet	# fifos
LDADD+=-lrumpvfs -lrump -lrumpuser -lpthread			# base

.include <bsd.test.mk>

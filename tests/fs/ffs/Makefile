#	$NetBSD: Makefile,v 1.6 2010/03/29 13:26:32 pooka Exp $
#

TESTSDIR=	${TESTSBASE}/fs/ffs
WARNS=		4

TESTS_C=	t_renamerace
TESTS_C+=	t_fifos

LDADD+=-lrumpfs_ffs						# ffs
LDADD+=-lrumpdev_disk -lrumpdev					# disk device
LDADD+=-lrumpvfs_fifofs -lrumpnet_local -lrumpnet_net -lrumpnet	# fifos
LDADD+=-lrumpvfs -lrump -lrumpuser -lpthread			# base

.include <bsd.test.mk>

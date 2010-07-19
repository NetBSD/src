#	$NetBSD: Makefile,v 1.10 2010/07/19 16:22:05 pooka Exp $
#

TESTSDIR=	${TESTSBASE}/fs/ffs
WARNS=		4

TESTS_C+=	t_fifos
TESTS_C+=	t_snapshot
TESTS_C+=	t_mount

LDADD+=-lrumpfs_ffs						# ffs
LDADD+=-lrumpdev_fss						# snapshot dev
LDADD+=-lrumpdev_disk -lrumpdev					# disk device
LDADD+=-lrumpvfs_fifofs -lrumpnet_local -lrumpnet_net -lrumpnet	# fifos
LDADD+=-lrumpvfs -lrump -lrumpuser -lpthread			# base

LDFLAGS+=-L${.CURDIR}/../common
LDADD+=-lvfstest

.include <bsd.test.mk>

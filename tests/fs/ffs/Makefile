#	$NetBSD: Makefile,v 1.13 2010/11/05 11:32:09 pooka Exp $
#

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/fs/ffs
WARNS=		4

TESTS_C+=	t_fifos
TESTS_C+=	t_snapshot
TESTS_C+=	t_snapshot_log
TESTS_C+=	t_mount

LDADD+=-lrumpfs_ffs						# ffs
LDADD+=-lrumpdev_fss						# snapshot dev
LDADD+=-lrumpdev_disk -lrumpdev					# disk device
LDADD+=-lrumpvfs_fifofs -lrumpnet_local -lrumpnet_net -lrumpnet	# fifos
VFSTESTDIR != cd ${.CURDIR}/../common && ${PRINTOBJDIR}
LDADD+=-L${VFSTESTDIR} -lvfstest
LDADD+=-lrumpvfs -lrump -lrumpuser -lpthread			# base

.include <bsd.test.mk>

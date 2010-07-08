#	$NetBSD: Makefile,v 1.2 2010/07/08 13:21:02 pooka Exp $
#

TESTSDIR=	${TESTSBASE}/fs/vfs
WARNS=		4

TESTS_C=	t_rmdirrace

LDADD+=-lrumpfs_ext2fs						# ext2fs
LDADD+=-lrumpfs_ffs						# ffs
LDADD+=-lrumpfs_lfs						# lfs
LDADD+=-lrumpfs_msdos						# msdos
LDADD+=-lrumpfs_sysvbfs						# sysvbfs
LDADD+=-lrumpfs_tmpfs						# tmpfs
LDADD+=-lrumpdev_disk -lrumpdev					# disk device
LDADD+=-lrumpvfs -lrump -lrumpuser -lpthread			# base

.include <bsd.test.mk>

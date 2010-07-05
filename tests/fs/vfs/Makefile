#	$NetBSD: Makefile,v 1.1 2010/07/05 12:53:58 njoly Exp $
#

TESTSDIR=	${TESTSBASE}/fs/vfs
WARNS=		4

TESTS_C=	t_rmdirrace

LDADD+=-lrumpfs_ext2fs						# ext2fs
LDADD+=-lrumpfs_ffs						# ffs
LDADD+=-lrumpfs_msdos						# msdos
LDADD+=-lrumpfs_sysvbfs						# sysvbfs
LDADD+=-lrumpfs_tmpfs						# tmpfs
LDADD+=-lrumpdev_disk -lrumpdev					# disk device
LDADD+=-lrumpvfs -lrump -lrumpuser -lpthread			# base

.include <bsd.test.mk>

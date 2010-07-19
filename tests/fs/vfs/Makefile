#	$NetBSD: Makefile,v 1.8 2010/07/19 16:32:01 pooka Exp $
#

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/fs/vfs
WARNS=		4

TESTS_C+=	t_renamerace
TESTS_C+=	t_rmdirrace
TESTS_C+=	t_vfsops
TESTS_C+=	t_vnops

LDADD+=-lrumpfs_ext2fs						# ext2fs
LDADD+=-lrumpfs_ffs						# ffs
LDADD+=-lrumpfs_lfs						# lfs
LDADD+=-lrumpfs_msdos						# msdos
LDADD+=-lrumpfs_syspuffs					# puffs
LDADD+=-lrumpdev_putter -lrumpdev				#   \ putter
LDADD+=-lrumpfs_sysvbfs						# sysvbfs
LDADD+=-lrumpfs_tmpfs						# tmpfs
LDADD+=-lrumpdev_disk -lrumpdev					# disk device
LDADD+=-lrumpvfs -lrump -lrumpuser -lpthread			# base

VFSTESTDIR != cd ${.CURDIR}/../common && ${PRINTOBJDIR}
LDADD+=-L${VFSTESTDIR} -lvfstest

.include <bsd.test.mk>

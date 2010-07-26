#	$NetBSD: Makefile,v 1.9 2010/07/26 16:25:19 pooka Exp $
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
LDADD+=-lrumpfs_nfs						# NFS
LDADD+=-lrumpfs_syspuffs					# puffs
LDADD+=-lrumpdev_putter -lrumpdev				#   \ putter
LDADD+=-lrumpfs_sysvbfs						# sysvbfs
LDADD+=-lrumpfs_tmpfs						# tmpfs
LDADD+=-lrumpdev_disk -lrumpdev					# disk device
LDADD+=-lrumpnet_shmif -lrumpnet_netinet -lrumpnet_net -lrumpnet
LDADD+=-lrumpvfs -lrump -lrumpuser -lpthread			# base

LDADD+=-lutil

VFSTESTDIR != cd ${.CURDIR}/../common && ${PRINTOBJDIR}
LDADD+=-L${VFSTESTDIR} -lvfstest

.include <bsd.test.mk>

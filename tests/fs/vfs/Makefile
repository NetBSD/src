#	$NetBSD: Makefile,v 1.29 2022/05/27 07:28:21 nia Exp $
#

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/fs/vfs
WARNS=		4

TESTS_C+=	t_link
TESTS_C+=	t_full
TESTS_C+=	t_io
TESTS_C+=	t_renamerace
TESTS_C+=	t_ro
TESTS_C+=	t_rmdirrace
TESTS_C+=	t_rwtoro
TESTS_C+=	t_union
TESTS_C+=	t_unpriv
TESTS_C+=	t_mtime_otrunc
TESTS_C+=	t_mtime_write
TESTS_C+=	t_vfsops
TESTS_C+=	t_vnops


.PATH: ${NETBSDSRCDIR}/lib/libc/gen
CPPFLAGS.sysctlbyname.c += -DRUMP_ACTION
CPPFLAGS.sysctlgetmibinfo.c += -DRUMP_ACTION
SRCS.t_link+= sysctlbyname.c sysctlgetmibinfo.c t_link.c

LDADD+=-lrumpnet_shmif -lrumpnet -lrumpnet_net -lrumpnet_netinet# TCP/IP
LDADD+=-lrumpfs_nfs						# NFS
LDADD+=-lrumpfs_ext2fs						# ext2fs
LDADD+=-lrumpfs_ffs						# ffs
LDADD+=-lrumpfs_msdos						# msdos
LDADD+=-lrumpfs_syspuffs					# puffs
LDADD+=-lrumpdev_putter -lrumpdev				#   \ putter
LDADD+=-lrumpfs_sysvbfs						# sysvbfs
LDADD+=-lrumpfs_tmpfs						# tmpfs
LDADD+=-lrumpfs_udf						# udf
LDADD+=-lrumpfs_union						# union
LDADD+=-lrumpfs_v7fs						# v7fs
LDADD+=-lrumpdev_disk -lrumpdev					# disk device
LDADD+=-lrumpfs_null -lrumpvfs_layerfs				# nullfs


VFSTESTDIR != cd ${.CURDIR}/../common && ${PRINTOBJDIR}
LDADD+=-L${VFSTESTDIR} -lvfstest

LDADD+= ${LIBRUMPBASE}						# base
LDADD+=-lrumpnet						# static linking

LDADD+=-lutil

.if (${MKLFS} != "no")
LDADD+=-lrumpfs_lfs
.endif

.if (${MKZFS} != "no")
LDADD+=-lrumpfs_zfs -lrumpkern_solaris -lrumpdev_rnd -lrumpkern_sysproxy
CPPFLAGS+=-DWANT_ZFS_TESTS
.endif

.include <bsd.test.mk>

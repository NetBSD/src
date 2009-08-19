#	$NetBSD: Makefile,v 1.2.2.2 2009/08/19 18:48:23 yamt Exp $

.include "../Makefile.inc"

KMOD=	zfs

ZFSDIR=	${S}/../external/cddl/osnet

.PATH:	${ZFSDIR}/dist/common/zfs

SRCS+=	zfs_comutil.c
SRCS+=	zfs_deleg.c
SRCS+=	zfs_namecheck.c
SRCS+=	zfs_prop.c
SRCS+=	zpool_prop.c
SRCS+=	zprop_common.c

.PATH:	${ZFSDIR}/dist/uts/common/fs

#SRCS+=	gfs.c

.PATH: ${ZFSDIR}/dist/uts/common/zmod

SRCS+= trees.c

.PATH:	${ZFSDIR}/dist/uts/common/fs/zfs

SRCS+=	arc.c
SRCS+=	bplist.c
SRCS+=	dbuf.c
SRCS+=	dmu.c
SRCS+=	dmu_object.c
SRCS+=	dmu_objset.c
SRCS+=	dmu_send.c
SRCS+=	dmu_traverse.c
SRCS+=	dmu_tx.c
SRCS+=	dmu_zfetch.c
SRCS+=	dnode.c
SRCS+=	dnode_sync.c
SRCS+=	dsl_dataset.c
SRCS+=	dsl_deleg.c
SRCS+=	dsl_dir.c
SRCS+=	dsl_pool.c
SRCS+=	dsl_prop.c
SRCS+=	dsl_scrub.c
SRCS+=	dsl_synctask.c
SRCS+=	fletcher.c
SRCS+=	gzip.c
SRCS+=	lzjb.c
SRCS+=	metaslab.c
SRCS+=	refcount.c
SRCS+=	rrwlock.c
SRCS+=	sha256.c
SRCS+=	spa.c
SRCS+=	spa_config.c
SRCS+=	spa_errlog.c
SRCS+=	spa_history.c
SRCS+=	spa_misc.c
SRCS+=	space_map.c
SRCS+=	txg.c
SRCS+=	uberblock.c
SRCS+=	unique.c
SRCS+=	vdev.c
SRCS+=	vdev_cache.c
SRCS+=	vdev_disk.c
SRCS+=	vdev_file.c
SRCS+=	vdev_label.c
SRCS+=	vdev_mirror.c
SRCS+=	vdev_missing.c
SRCS+=	vdev_queue.c
SRCS+=	vdev_raidz.c
SRCS+=	vdev_root.c
SRCS+=	zap.c
SRCS+=	zap_leaf.c
SRCS+=	zap_micro.c
SRCS+=	zfs_byteswap.c
#SRCS+=	zfs_ctldir.c
SRCS+=	zfs_dir.c
SRCS+=	zfs_fuid.c
SRCS+=	zfs_fm.c
SRCS+=	zfs_ioctl.c
SRCS+=	zfs_log.c
SRCS+=	zfs_replay.c
SRCS+=	zfs_rlock.c
SRCS+=	zfs_vfsops.c
SRCS+=	zfs_vnops.c
SRCS+=	zfs_znode.c
SRCS+= 	zfs_acl.c
SRCS+=	zil.c
SRCS+=	zio.c
SRCS+=	zio_checksum.c
SRCS+=	zio_compress.c
SRCS+=	zio_inject.c
SRCS+=	zutil.c
SRCS+=	zvol.c

.PATH: ${ZFSDIR}/sys/kern

SRCS+= zfs_stub.c

WARNS=		0
NOGCCERROR=	yes

CWARNFLAGS+=	-Wall
CWARNFLAGS+=	-Wno-unknown-pragmas
CWARNFLAGS+=	-Wno-missing-braces
CWARNFLAGS+=	-Wno-parentheses
CWARNFLAGS+=	-Wno-uninitialized
CWARNFLAGS+=	-Wno-unused
CWARNFLAGS+=	-Wno-switch
CWARNFLAGS+=	-Wno-strict-prototypes
CWARNFLAGS+=	-Wno-missing-prototypes
CWARNFLAGS+=	-Wno-format

CPPFLAGS+=	-I${ZFSDIR}/sys
CPPFLAGS+=	-I${ZFSDIR}/dist/common/acl
CPPFLAGS+=	-I${ZFSDIR}/dist/common/zfs
CPPFLAGS+=	-I${ZFSDIR}/dist/uts/common/fs/zfs
CPPFLAGS+=	-I${ZFSDIR}/dist/uts/common/zmod
CPPFLAGS+=	-I${ZFSDIR}/dist/uts/common
CPPFLAGS+=	-I${ZFSDIR}/dist/uts/common/zfs
CPPFLAGS+=	-I${ZFSDIR}/dist/uts/common

CPPFLAGS+=	-Dcaddr_t=__caddr_t "-D__va_list=va_list" -D__NetBSD__
CPPFLAGS+=	-std=c99
CPPFLAGS+=	-D_NFS_NFS_H_
CPPFLAGS+=	-D_PROPLIB_ZFS_CONFLICT 

CFLAGS+=	-DDIAGNOSTIC -g -O0
# -DZFS_DEBUG

.include <bsd.kmodule.mk>

#	$NetBSD: Makefile,v 1.8.12.1 2018/06/25 07:25:25 pgoyette Exp $

.include <bsd.init.mk>

.PATH: ${.CURDIR}/../../dist/common/zfs
.PATH: ${.CURDIR}/../../dist/common/fs/zfs
.PATH: ${.CURDIR}/../../dist/lib/libzfs/common

LIB=	zfs

LIBDPLIBS+= \
	avl	${.CURDIR}/../libavl \
	nvpair	${.CURDIR}/../libnvpair \
	m	${NETBSDSRCDIR}/lib/libm \
	pthread	${NETBSDSRCDIR}/lib/libpthread \
	umem	${.CURDIR}/../libumem \
	util	${NETBSDSRCDIR}/lib/libutil \
	uutil	${.CURDIR}/../libuutil \
	z	${NETBSDSRCDIR}/lib/libz \
	zfs_core ${.CURDIR}/../libzfs_core

# Local
SRCS+=	deviceid.c
SRCS+=	fsshare.c
SRCS+=	mkdirp.c
SRCS+=	mnttab.c
SRCS+=	thread_pool.c
SRCS+=	zmount.c
SRCS+=	zone.c

# Sun
SRCS+=	libzfs_changelist.c
SRCS+=	libzfs_config.c
SRCS+=	libzfs_dataset.c
SRCS+=	libzfs_diff.c
#SRCS+=	libzfs_fru.c
#SRCS+=	libzfs_graph.c
SRCS+=	libzfs_import.c
SRCS+=	libzfs_iter.c
SRCS+=	libzfs_mount.c
SRCS+=	libzfs_pool.c
SRCS+=	libzfs_sendrecv.c
SRCS+=	libzfs_status.c
SRCS+=	libzfs_util.c
SRCS+=	zfeature_common.c
SRCS+=	zfs_comutil.c
SRCS+=	zfs_deleg.c
SRCS+=	zfs_fletcher.c
SRCS+=	zfs_namecheck.c
SRCS+=	zfs_prop.c
SRCS+=	zpool_prop.c
SRCS+=	zprop_common.c

CPPFLAGS+=	-I${NETBSDSRCDIR}/sbin/mount

# Avoid conflict with libprop
CPPFLAGS+=	-D_PROPLIB_ZFS_CONFLICT

# Avoid internal header conflicts
CPPFLAGS+=	-D_ZFS_CONTEXT_NO_VERIFY -D_ZFS_CONTEXT_NO_DEBUG

CWARNFLAGS+=	-Wno-parentheses

.include "../../Makefile.zfs"
.include <bsd.lib.mk>

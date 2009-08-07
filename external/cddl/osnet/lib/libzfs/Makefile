#	$NetBSD: Makefile,v 1.1 2009/08/07 20:57:56 haad Exp $

.PATH: ${.CURDIR}/../../dist/common/zfs
.PATH: ${.CURDIR}/../../dist/common/fs/zfs
.PATH: ${.CURDIR}/../../dist/lib/libzfs/common

LIB=	zfs
DPADD=	${LIBUTIL}
LDADD=	-lutil

# Local
SRCS=	deviceid.c mnttab.c mkdirp.c zmount.c fsshare.c zone.c efi.c ucred.c

# Sun
SRCS+=	zfs_namecheck.c zprop_common.c zfs_prop.c zpool_prop.c
SRCS+=	zfs_deleg.c zfs_comutil.c
SRCS+=	libzfs_dataset.c libzfs_util.c libzfs_graph.c libzfs_mount.c
SRCS+=	libzfs_pool.c libzfs_changelist.c libzfs_config.c libzfs_import.c
SRCS+=	libzfs_status.c libzfs_sendrecv.c

CPPFLAGS+=	-I${NETBSDSRCDIR}/sbin/mount

# Avoid conflict with libprop
CPPFLAGS+=	-D_PROPLIB_ZFS_CONFLICT

# Avoid internal header conflicts
CPPFLAGS+=	-D_ZFS_CONTEXT_NO_VERIFY

.include "../../Makefile.zfs"
.include <bsd.lib.mk>

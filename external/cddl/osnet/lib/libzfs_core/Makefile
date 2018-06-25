#	$NetBSD: Makefile,v 1.1.2.2 2018/06/25 07:25:25 pgoyette Exp $

.include <bsd.init.mk>

.PATH: ${.CURDIR}/../../dist/common/zfs
.PATH: ${.CURDIR}/../../dist/common/fs/zfs
.PATH: ${.CURDIR}/../../dist/lib/libzfs/common
.PATH: ${.CURDIR}/../../dist/lib/libzfs_core/common

LIB=	zfs_core

LIBDPLIBS+= \
	nvpair	${.CURDIR}/../libnvpair


SRCS=	libzfs_core.c \
	libzfs_core_compat.c \
	zfs_ioctl_compat.c

SRCS+=	libzfs_compat.c

.include "../../Makefile.zfs"
.include <bsd.lib.mk>


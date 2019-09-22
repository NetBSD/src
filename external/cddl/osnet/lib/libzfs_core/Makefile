#	$NetBSD: Makefile,v 1.2 2019/09/22 18:45:20 brad Exp $

.include <bsd.init.mk>

USE_SHLIBDIR=	yes

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


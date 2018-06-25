#	$NetBSD: Makefile,v 1.11.12.1 2018/06/25 07:25:25 pgoyette Exp $

.include <bsd.init.mk>

.include "${.CURDIR}/../../dist/uts/common/Makefile.files"
.include "../../Makefile.zfs"

.PATH: ${.CURDIR}/../../dist/common/zfs
.PATH: ${.CURDIR}/../../dist/common/unicode
.PATH: ${.CURDIR}/../../dist/lib/libzpool/common
.PATH: ${.CURDIR}/../../dist/uts/common/fs/zfs
.PATH: ${.CURDIR}/../../dist/uts/common/os

LIBDPLIBS+= \
	avl	${.CURDIR}/../libavl \
	nvpair	${.CURDIR}/../libnvpair \
	pthread	${NETBSDSRCDIR}/lib/libpthread \
	umem	${.CURDIR}/../libumem \
	z	${NETBSDSRCDIR}/lib/libz \

LIB=		zpool
CPPFLAGS+=	-std=c99

CWARNFLAGS+=	-Wno-missing-field-initializers
CWARNFLAGS+=	-Wno-parentheses

# Local stuff
SRCS+=		kernel.c atomic.c

# Sun stuff
SRCS+=		${ZFS_COMMON_OBJS:C/.o$/.c/} vdev_file.c trim_map.c
SRCS+=		${ZFS_SHARED_OBJS:C/.o$/.c/}
SRCS+=		taskq.c util.c list.c u8_textprep.c

print:
	echo ${LIST_SRCS} ${LIST_OBJS}


.include <bsd.lib.mk>

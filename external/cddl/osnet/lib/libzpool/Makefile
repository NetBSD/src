#	$NetBSD: Makefile,v 1.1 2009/08/07 20:57:56 haad Exp $

.include "${.CURDIR}/../../dist/uts/common/Makefile.files"

.PATH: ${.CURDIR}/../../dist/common/zfs
.PATH: ${.CURDIR}/../../dist/common/unicode
.PATH: ${.CURDIR}/../../dist/lib/libzpool/common
.PATH: ${.CURDIR}/../../dist/uts/common/fs/zfs
.PATH: ${.CURDIR}/../../dist/uts/common/os

LIB=		zpool
DPADD=		${LIBPTHREAD} ${LIBZ}
LDADD=		-lpthread -lz
CPPFLAGS+=	-std=c99

# Local stuff
SRCS=		kernel.c kernel2.c

# Sun stuff
SRCS+=		${ZFS_COMMON_OBJS:C/.o$/.c/}
SRCS+=		${ZFS_SHARED_OBJS:C/.o$/.c/}
SRCS+=		taskq.c util.c list.o u8_textprep.o

print:
	echo ${LIST_SRCS} ${LIST_OBJS}

.include "../../Makefile.zfs"
.include <bsd.lib.mk>

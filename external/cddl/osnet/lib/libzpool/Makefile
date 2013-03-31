#	$NetBSD: Makefile,v 1.6.18.2 2013/03/31 23:06:42 agc Exp $

.include <bsd.own.mk>

.include "${.CURDIR}/../../dist/uts/common/Makefile.files"
.include "../../Makefile.zfs"

.PATH: ${.CURDIR}/../../dist/common/zfs
.PATH: ${.CURDIR}/../../dist/common/unicode
.PATH: ${.CURDIR}/../../dist/lib/libzpool/common
.PATH: ${.CURDIR}/../../dist/uts/common/fs/zfs
.PATH: ${.CURDIR}/../../dist/uts/common/os

LIB=		zpool
LIBDPLIBS+=	z		${NETBSDSRCDIR}/lib/libz
LIBDPLIBS+=	pthread		${NETBSDSRCDIR}/lib/libpthread
CPPFLAGS+=	-std=c99
#CPPFLAGS+=-Wall -Wno-unknown-pragmas

# Local stuff
SRCS=		kernel.c kernel2.c

# Sun stuff
SRCS+=		${ZFS_COMMON_OBJS:C/.o$/.c/}
SRCS+=		${ZFS_SHARED_OBJS:C/.o$/.c/}
SRCS+=		taskq.c util.c list.o u8_textprep.o

print:
	echo ${LIST_SRCS} ${LIST_OBJS}


.include <bsd.lib.mk>

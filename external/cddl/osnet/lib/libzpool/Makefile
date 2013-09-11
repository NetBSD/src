#	$NetBSD: Makefile,v 1.8 2013/09/11 23:04:10 joerg Exp $

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
#CPPFLAGS+=-Wall -Wno-unknown-pragmas

# Local stuff
SRCS=		kernel.c kernel2.c
.if ${MACHINE_ARCH} == "arm"
SRCS+=		atomic.c
.endif

# Sun stuff
SRCS+=		${ZFS_COMMON_OBJS:C/.o$/.c/}
SRCS+=		${ZFS_SHARED_OBJS:C/.o$/.c/}
SRCS+=		taskq.c util.c list.o u8_textprep.o

print:
	echo ${LIST_SRCS} ${LIST_OBJS}


.include <bsd.lib.mk>

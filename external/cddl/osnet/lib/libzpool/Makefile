#	$NetBSD: Makefile,v 1.6.12.1 2013/06/23 06:28:31 tls Exp $

.include "${.CURDIR}/../../dist/uts/common/Makefile.files"
.include "../../Makefile.zfs"

.PATH: ${.CURDIR}/../../dist/common/zfs
.PATH: ${.CURDIR}/../../dist/common/unicode
.PATH: ${.CURDIR}/../../dist/lib/libzpool/common
.PATH: ${.CURDIR}/../../dist/uts/common/fs/zfs
.PATH: ${.CURDIR}/../../dist/uts/common/os

LIB=		zpool
DPADD=		${LIBPTHREAD} ${LIBZ}
LDADD=		-lpthread -lz
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

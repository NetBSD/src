#	$NetBSD: Makefile,v 1.2 2009/10/06 16:28:10 haad Exp $

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

# Local stuff
SRCS=		kernel.c kernel2.c

# Sun stuff
SRCS+=		${ZFS_COMMON_OBJS:C/.o$/.c/}
SRCS+=		${ZFS_SHARED_OBJS:C/.o$/.c/}
SRCS+=		taskq.c util.c list.o u8_textprep.o

# Add opensolaris atomic functions and use fakedones if we do not support them
.if ${MACHINE_ARCH} == "i386"
CPPFLAGS+=	-D__HAVE_ATOMIC64_OPS	# add NetBSD 64 add operations on i386
.PATH: ${NETBSDSRCDIR}/common/lib/libc/atomic
SRCS+= atomic_add_64_cas.c atomic_add_64_nv_cas.c atomic_inc_64_add.c atomic_dec_64_add.c
.endif

print:
	echo ${LIST_SRCS} ${LIST_OBJS}


.include <bsd.lib.mk>

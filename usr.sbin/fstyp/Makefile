#	$NetBSD: Makefile,v 1.2 2018/01/22 01:34:42 joerg Exp $

.include <bsd.own.mk>

PROG=	fstyp
SRCS=	cd9660.c ext2fs.c fstyp.c msdosfs.c ntfs.c ufs.c

.if (${MKZFS} != "no")
SRCS+=	zfs.c
.endif

MAN=	fstyp.8

WARNS?=	6

.if (${MKZFS} != "no")
CFLAGS+=	-DHAVE_ZFS
CFLAGS+=	-Wno-unknown-pragmas
CFLAGS+=	-I${NETBSDSRCDIR}/external
CFLAGS+=	-I${NETBSDSRCDIR}/external/cddl/osnet/include
CFLAGS+=	-I${NETBSDSRCDIR}/external/cddl/osnet/sys
CFLAGS+=	-I${NETBSDSRCDIR}/external/cddl/osnet/dist/uts/common
CFLAGS+=	-I${NETBSDSRCDIR}/external/cddl/osnet/dist/uts/common/fs/zfs
CFLAGS+=	-I${NETBSDSRCDIR}/external/cddl/osnet/dist/lib/libnvpair
LDADD+=	-lnvpair -lzfs
.endif

CWARNFLAGS.clang+=	-Wno-unknown-pragmas

.include <bsd.prog.mk>

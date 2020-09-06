#	$NetBSD: Makefile,v 1.10 2020/09/06 07:20:31 mrg Exp $

.include <bsd.own.mk>

PROG=	fstyp
SRCS=	apfs.c cd9660.c exfat.c ext2fs.c fstyp.c hammer.c hammer2.c hfsplus.c msdosfs.c ntfs.c ufs.c

.if (${MKZFS} != "no")
SRCS+=	zfs.c
.endif

MAN=	fstyp.8

WARNS?=	6

.if (${MKZFS} != "no")
CFLAGS+=	-DHAVE_ZFS
COPTS.zfs.c+=	-Wno-unknown-pragmas
COPTS.zfs.c+=	-Wno-sign-conversion
COPTS.zfs.c+=	-Wno-strict-prototypes
COPTS.zfs.c+=	-I${NETBSDSRCDIR}/external
COPTS.zfs.c+=	-I${NETBSDSRCDIR}/external/cddl/osnet/include
COPTS.zfs.c+=	-I${NETBSDSRCDIR}/external/cddl/osnet/sys
COPTS.zfs.c+=	-I${NETBSDSRCDIR}/external/cddl/osnet/dist/head
COPTS.zfs.c+=	-I${NETBSDSRCDIR}/external/cddl/osnet/dist/lib/libzpool/common
COPTS.zfs.c+=	-I${NETBSDSRCDIR}/external/cddl/osnet/dist/uts/common
COPTS.zfs.c+=	-I${NETBSDSRCDIR}/external/cddl/osnet/dist/uts/common/fs/zfs
COPTS.zfs.c+=	-I${NETBSDSRCDIR}/external/cddl/osnet/dist/lib/libnvpair
LDADD+=	-lnvpair -lzfs
.endif

CWARNFLAGS.clang+=	-Wno-unknown-pragmas
CWARNFLAGS.gcc+=        ${GCC_NO_ADDR_OF_PACKED_MEMBER}

.include <bsd.prog.mk>

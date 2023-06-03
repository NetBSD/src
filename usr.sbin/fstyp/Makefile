#	$NetBSD: Makefile,v 1.14 2023/06/03 21:26:29 lukem Exp $

.include <bsd.own.mk>

PROG=	fstyp
SRCS=	apfs.c cd9660.c exfat.c ext2fs.c fstyp.c hammer.c hammer2.c hfsplus.c msdosfs.c ntfs.c ufs.c

.if (${MKZFS} != "no")
SRCS+=	zfs.c
.endif

MAN=	fstyp.8

WARNS?=	6

.if (${MKZFS} != "no")
COPTS.zfs.c+=	-Wno-unknown-pragmas
COPTS.zfs.c+=	-Wno-sign-conversion
COPTS.zfs.c+=	-Wno-strict-prototypes
LDADD+=	-lnvpair -lzfs

OSNET=${NETBSDSRCDIR}/external/cddl/osnet
CPPFLAGS+=	-DHAVE_ZFS
CPPFLAGS.zfs.c+=	-I${NETBSDSRCDIR}/external
CPPFLAGS.zfs.c+=	-I${OSNET}/include
CPPFLAGS.zfs.c+=	-I${OSNET}/sys
CPPFLAGS.zfs.c+=	-I${OSNET}/dist/head
CPPFLAGS.zfs.c+=	-I${OSNET}/dist/lib/libzpool/common
CPPFLAGS.zfs.c+=	-I${OSNET}/dist/uts/common
CPPFLAGS.zfs.c+=	-I${OSNET}/dist/uts/common/fs/zfs
CPPFLAGS.zfs.c+=	-I${OSNET}/dist/lib/libnvpair
.endif

CWARNFLAGS.clang+=	-Wno-unknown-pragmas
CWARNFLAGS+=        	${CC_WNO_ADDRESS_OF_PACKED_MEMBER}

.include <bsd.prog.mk>

#	$NetBSD: Makefile,v 1.19 2022/01/06 15:08:56 pgoyette Exp $

.include "../Makefile.inc"

.PATH:	${S}/ufs/ffs

KMOD=	ffs
CPPFLAGS+=	-DFFS_EI -DWAPBL -DAPPLE_UFS -DQUOTA -DQUOTA2
CPPFLAGS+=	-DUFS_EXTATTR -DUFS_ACL
#CPPFLAGS+=	-DUFS_DIRHASH # disabled - suspected of corrupting memory

CWARNFLAGS.clang=	-Wno-conversion
COPTS.ffs_appleufs.c+=   ${NO_ADDR_OF_PACKED_MEMBER}


SRCS+=	ffs_alloc.c ffs_balloc.c ffs_inode.c ffs_subr.c ffs_tables.c \
	ffs_vfsops.c ffs_vnops.c ffs_snapshot.c ffs_extattr.c \
	ffs_bswap.c ffs_wapbl.c ffs_appleufs.c ffs_quota2.c

WARNS=	3

.include <bsd.kmodule.mk>

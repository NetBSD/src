#	$NetBSD: Makefile,v 1.17 2020/10/18 15:02:12 christos Exp $

.include "../Makefile.inc"

.PATH:	${S}/ufs/ffs

KMOD=	ffs
CPPFLAGS+=      -DUFS_DIRHASH -DFFS_EI -DWAPBL -DAPPLE_UFS -DQUOTA -DQUOTA2
CPPFLAGS+=	-DUFS_EXTATTR -DUFS_ACL

CWARNFLAGS.clang=	-Wno-conversion
CWARNFLAGS.ffs_appleufs.c+=   ${GCC_NO_ADDR_OF_PACKED_MEMBER}


SRCS+=	ffs_alloc.c ffs_balloc.c ffs_inode.c ffs_subr.c ffs_tables.c \
	ffs_vfsops.c ffs_vnops.c ffs_snapshot.c ffs_extattr.c \
	ffs_bswap.c ffs_wapbl.c ffs_appleufs.c ffs_quota2.c

WARNS=	3

.include <bsd.kmodule.mk>

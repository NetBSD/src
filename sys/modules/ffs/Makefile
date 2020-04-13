#	$NetBSD: Makefile,v 1.9.28.2 2020/04/13 08:05:11 martin Exp $

.include "../Makefile.inc"

.PATH:	${S}/ufs/ffs

KMOD=	ffs
CPPFLAGS+=      -DUFS_DIRHASH -DFFS_EI -DWAPBL -DAPPLE_UFS -DQUOTA -DQUOTA2
CPPFLAGS+=	-DUFS_EXTATTR

CWARNFLAGS.clang=	-Wno-conversion

SRCS+=	ffs_alloc.c ffs_balloc.c ffs_inode.c ffs_subr.c ffs_tables.c \
	ffs_vfsops.c ffs_vnops.c ffs_snapshot.c \
	ffs_bswap.c ffs_wapbl.c ffs_appleufs.c ffs_quota2.c

WARNS=	3

.include <bsd.kmodule.mk>

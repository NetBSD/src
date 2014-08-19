#	$NetBSD: Makefile,v 1.8.2.1 2014/08/20 00:04:33 tls Exp $

.include "../Makefile.inc"

.PATH:	${S}/ufs/ffs

KMOD=	ffs
CPPFLAGS+=      -DUFS_DIRHASH -DFFS_EI -DWAPBL -DAPPLE_UFS -DQUOTA -DQUOTA2

CWARNFLAGS.clang=	-Wno-conversion

.PATH:	${S}/ufs/ufs
SRCS=	ufs_bmap.c ufs_dirhash.c ufs_extattr.c ufs_inode.c \
	ufs_lookup.c ufs_quota.c ufs_quota1.c ufs_quota2.c ufs_rename.c \
	ufs_vfsops.c ufs_vnops.c ufs_wapbl.c quota2_subr.c

.PATH:	${S}/ufs/ffs
SRCS+=	ffs_alloc.c ffs_balloc.c ffs_inode.c ffs_subr.c ffs_tables.c \
	ffs_vfsops.c ffs_vnops.c ffs_snapshot.c \
	ffs_bswap.c ffs_wapbl.c ffs_appleufs.c ffs_quota2.c

.include <bsd.kmodule.mk>

#	$NetBSD: Makefile,v 1.2.4.1 2009/05/13 17:22:19 jym Exp $

.include "../Makefile.inc"

.PATH:	${S}/ufs/ffs

KMOD=	ffs
CPPFLAGS+=      -DUFS_DIRHASH -DFFS_EI -DWAPBL -DAPPLE_UFS -DQUOTA -DFFS

.PATH:	${S}/ufs/ufs
SRCS=	ufs_bmap.c ufs_dirhash.c ufs_extattr.c ufs_ihash.c ufs_inode.c \
	ufs_lookup.c ufs_quota.c ufs_vfsops.c ufs_vnops.c ufs_wapbl.c

.PATH:	${S}/ufs/ffs
SRCS+=	ffs_alloc.c ffs_balloc.c ffs_inode.c ffs_subr.c ffs_tables.c \
	ffs_vfsops.c ffs_vnops.c ffs_snapshot.c \
	ffs_bswap.c ffs_wapbl.c ffs_appleufs.c

.include <bsd.kmodule.mk>

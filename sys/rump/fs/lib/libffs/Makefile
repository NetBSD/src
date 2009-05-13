#	$NetBSD: Makefile,v 1.6.8.1 2009/05/13 17:22:57 jym Exp $
#

.PATH:  ${.CURDIR}/../../../../ufs/ffs ${.CURDIR}/../../../../ufs/ufs

LIB=	rumpfs_ffs

SRCS=	ffs_alloc.c ffs_appleufs.c ffs_balloc.c ffs_bswap.c ffs_inode.c	\
	ffs_snapshot.c ffs_subr.c ffs_tables.c ffs_vfsops.c ffs_vnops.c	\
	ffs_wapbl.c

SRCS+=	ufs_bmap.c ufs_dirhash.c ufs_ihash.c ufs_inode.c ufs_lookup.c	\
	ufs_vfsops.c ufs_vnops.c ufs_wapbl.c

CPPFLAGS+=	-DFFS_EI -DWAPBL -DAPPLE_UFS
CPPFLAGS+=	-DUFS_DIRHASH -DWAPBL -DAPPLE_UFS
CFLAGS+=	-Wno-pointer-sign

.include <bsd.lib.mk>
.include <bsd.klinks.mk>

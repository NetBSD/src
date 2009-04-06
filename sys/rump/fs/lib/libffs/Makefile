#	$NetBSD: Makefile,v 1.8 2009/04/06 20:46:44 pooka Exp $
#

.PATH:  ${.CURDIR}/../../../../ufs/ffs

LIB=	rumpfs_ffs

SRCS=	ffs_alloc.c ffs_appleufs.c ffs_balloc.c ffs_bswap.c ffs_inode.c	\
	ffs_snapshot.c ffs_subr.c ffs_tables.c ffs_vfsops.c ffs_vnops.c	\
	ffs_wapbl.c

CPPFLAGS+=	-DFFS_EI -DWAPBL -DAPPLE_UFS
CFLAGS+=	-Wno-pointer-sign

.include <bsd.lib.mk>
.include <bsd.klinks.mk>

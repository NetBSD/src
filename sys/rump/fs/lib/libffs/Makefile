#	$NetBSD: Makefile,v 1.6.2.2 2009/04/28 07:37:50 skrll Exp $
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

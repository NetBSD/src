#	$NetBSD: Makefile,v 1.4.20.1 2008/09/28 10:41:03 mjf Exp $
#

.PATH:  ${.CURDIR}/../../../../ufs/ufs

LIB=	rumpfs_ufs

SRCS=	ufs_bmap.c ufs_dirhash.c ufs_ihash.c ufs_inode.c ufs_lookup.c	\
	ufs_vfsops.c ufs_vnops.c ufs_wapbl.c

CPPFLAGS+=	-DUFS_DIRHASH -DFFS_EI -DWAPBL -DAPPLE_UFS

.include <bsd.lib.mk>
.include <bsd.klinks.mk>

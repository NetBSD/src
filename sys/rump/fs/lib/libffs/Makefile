#	$NetBSD: Makefile,v 1.14.12.1 2014/08/10 06:56:50 tls Exp $
#

.PATH:  ${.CURDIR}/../../../../ufs/ffs ${.CURDIR}/../../../../ufs/ufs

LIB=	rumpfs_ffs

SRCS=	ffs_alloc.c ffs_appleufs.c ffs_balloc.c ffs_bswap.c ffs_inode.c	\
	ffs_snapshot.c ffs_subr.c ffs_tables.c ffs_vfsops.c ffs_vnops.c	\
	ffs_wapbl.c ffs_quota2.c

SRCS+=	ufs_bmap.c ufs_dirhash.c ufs_extattr.c ufs_inode.c	\
	ufs_lookup.c ufs_rename.c ufs_vfsops.c ufs_vnops.c ufs_wapbl.c \
	ufs_quota.c ufs_quota2.c quota2_subr.c

CPPFLAGS+=	-DFFS_EI -DUFS_DIRHASH -DWAPBL -DAPPLE_UFS -DUFS_EXTATTR \
		-DQUOTA2

.include <bsd.lib.mk>
.include <bsd.klinks.mk>

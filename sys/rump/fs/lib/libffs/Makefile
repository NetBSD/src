#	$NetBSD: Makefile,v 1.3.26.1 2008/09/18 04:37:03 wrstuden Exp $
#

.PATH:  ${.CURDIR}/../../../../ufs/ffs

LIB=	rumpfs_ffs

SRCS=	ffs_alloc.c ffs_appleufs.c ffs_balloc.c ffs_bswap.c ffs_inode.c	\
	ffs_softdep.stub.c ffs_subr.c ffs_tables.c ffs_vfsops.c		\
	ffs_vnops.c ffs_snapshot.c ffs_wapbl.c

CPPFLAGS+=	-DFFS_NO_SNAPSHOT -DFFS_EI -DWAPBL -DAPPLE_UFS
CFLAGS+=	-Wno-pointer-sign

.include <bsd.lib.mk>
.include <bsd.klinks.mk>

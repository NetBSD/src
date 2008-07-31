#	$NetBSD: Makefile,v 1.5 2008/07/31 05:38:05 simonb Exp $
#

.PATH:  ${.CURDIR}/../../../../ufs/ffs

LIB=	rumpfs_ffs

SRCS=	ffs_alloc.c ffs_balloc.c ffs_bswap.c ffs_inode.c		\
	ffs_softdep.stub.c ffs_subr.c ffs_tables.c ffs_vfsops.c		\
	ffs_vnops.c ffs_snapshot.c ffs_wapbl.c

CPPFLAGS+=	-DFFS_NO_SNAPSHOT -DFFS_EI -DWAPBL
CFLAGS+=	-Wno-pointer-sign

.include <bsd.lib.mk>
.include <bsd.klinks.mk>

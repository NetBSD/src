#	$NetBSD: Makefile,v 1.1 2007/08/05 22:28:04 pooka Exp $
#

LIB=	ffs

.PATH:  ${NETBSDSRCDIR}/sys/ufs/ffs

SRCS=	ffs_alloc.c ffs_balloc.c ffs_bswap.c ffs_inode.c		\
	ffs_softdep.stub.c ffs_subr.c ffs_tables.c ffs_vfsops.c		\
	ffs_vnops.c ffs_snapshot.c

CPPFLAGS+=	-DFFS_NO_SNAPSHOT
CFLAGS+=	-Wno-pointer-sign

.include <bsd.lib.mk>
.include <bsd.klinks.mk>

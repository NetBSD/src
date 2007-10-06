#	$NetBSD: Makefile,v 1.2.10.1 2007/10/06 15:29:28 yamt Exp $
#

.include <bsd.own.mk>

LIB=	ffs

.PATH:  ${NETBSDSRCDIR}/sys/ufs/ffs

SRCS=	ffs_alloc.c ffs_balloc.c ffs_bswap.c ffs_inode.c		\
	ffs_softdep.stub.c ffs_subr.c ffs_tables.c ffs_vfsops.c		\
	ffs_vnops.c ffs_snapshot.c

CPPFLAGS+=	-DFFS_NO_SNAPSHOT -DFFS_EI
CFLAGS+=	-Wno-pointer-sign

.include <bsd.lib.mk>
.include <bsd.klinks.mk>

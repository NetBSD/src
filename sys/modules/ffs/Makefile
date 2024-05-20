#	$NetBSD: Makefile,v 1.25 2024/05/20 18:14:38 pgoyette Exp $

.include "../Makefile.inc"

.PATH:	${S}/ufs/ffs

KMOD=	ffs
CPPFLAGS+=	-DFFS_EI -DWAPBL -DAPPLE_UFS -DQUOTA -DQUOTA2
CPPFLAGS+=	-DUFS_DIRHASH

CWARNFLAGS.clang=	-Wno-conversion
COPTS.ffs_appleufs.c+=	${CC_WNO_ADDRESS_OF_PACKED_MEMBER}

SRCS+=	ffs_alloc.c ffs_balloc.c ffs_inode.c ffs_subr.c ffs_tables.c \
	ffs_vfsops.c ffs_vnops.c ffs_snapshot.c \
	ffs_bswap.c ffs_wapbl.c ffs_appleufs.c ffs_quota2.c

#CPPFLAGS+=	-DUFS_EXTATTR
#SRCS+=	ffs_extattr.c

#CPPFLAGS+=	-DUFS_ACL
#SRCS+=	ufs_acl.c

WARNS=	3

.include <bsd.kmodule.mk>

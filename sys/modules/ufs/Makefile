#	$NetBSD: Makefile,v 1.9 2024/05/20 18:14:38 pgoyette Exp $

.include "../Makefile.inc"

KMOD=	ufs

CPPFLAGS+=	-DFFS_EI -DWAPBL -DAPPLE_UFS -DQUOTA -DQUOTA2
CPPFLAGS+=	-DUFS_DIRHASH

CWARNFLAGS.clang=	-Wno-conversion

.PATH:	${S}/ufs/ufs
SRCS+=	ufs_bmap.c ufs_dirhash.c ufs_inode.c \
	ufs_lookup.c ufs_quota.c ufs_quota1.c ufs_quota2.c ufs_rename.c \
	ufs_vfsops.c ufs_vnops.c quota1_subr.c quota2_subr.c

#CPPFLAGS+=	-DUFS_ACL
#SRCS+=	ufs_acl.c

#CPPFLAGS+=	-DUFS_EXTATTR
#SRCS+=	ufs_extattr.c

WARNS=	3

.include <bsd.kmodule.mk>

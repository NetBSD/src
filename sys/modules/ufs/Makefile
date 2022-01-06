#	$NetBSD: Makefile,v 1.5 2022/01/06 15:08:56 pgoyette Exp $

.include "../Makefile.inc"

KMOD=	ufs

CPPFLAGS+=	-DFFS_EI -DWAPBL -DAPPLE_UFS -DQUOTA -DQUOTA2
CPPFLAGS+=	-DUFS_EXTATTR -DUFS_ACL
CPPFLAGS+=	-DUFS_DIRHASH	# disabled - suspected of corrupting memory

CWARNFLAGS.clang=	-Wno-conversion

.PATH:	${S}/ufs/ufs
SRCS=	ufs_acl.c ufs_bmap.c ufs_dirhash.c ufs_extattr.c ufs_inode.c \
	ufs_lookup.c ufs_quota.c ufs_quota1.c ufs_quota2.c ufs_rename.c \
	ufs_vfsops.c ufs_vnops.c quota1_subr.c quota2_subr.c

WARNS=	3

.include <bsd.kmodule.mk>

#	$NetBSD: Makefile,v 1.2 2019/08/19 09:31:31 christos Exp $

.include "../Makefile.inc"

KMOD=	ufs

CPPFLAGS+=      -DUFS_DIRHASH -DFFS_EI -DWAPBL -DAPPLE_UFS -DQUOTA -DQUOTA2
CPPFLAGS+=	-DUFS_EXTATTR

CWARNFLAGS.clang=	-Wno-conversion

.PATH:	${S}/ufs/ufs
SRCS=	ufs_bmap.c ufs_dirhash.c ufs_extattr.c ufs_inode.c \
	ufs_lookup.c ufs_quota.c ufs_quota1.c ufs_quota2.c ufs_rename.c \
	ufs_vfsops.c ufs_vnops.c ufs_wapbl.c quota1_subr.c quota2_subr.c

WARNS=	3

.include <bsd.kmodule.mk>

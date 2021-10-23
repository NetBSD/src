#	$NetBSD: Makefile,v 1.4 2021/10/23 07:41:37 hannken Exp $

.include "../Makefile.inc"

.PATH:	${S}/fs/msdosfs

KMOD=	msdos
SRCS=	msdosfs_conv.c msdosfs_denode.c msdosfs_fat.c msdosfs_lookup.c \
	msdosfs_rename.c msdosfs_vfsops.c msdosfs_vnops.c msdosfs_unicode.c

WARNS=	3

.include <bsd.kmodule.mk>

#	$NetBSD: Makefile,v 1.1.32.1 2017/12/03 11:38:58 jdolecek Exp $

.include "../Makefile.inc"

.PATH:	${S}/fs/msdosfs

KMOD=	msdos
SRCS=	msdosfs_conv.c msdosfs_denode.c msdosfs_fat.c msdosfs_lookup.c \
	msdosfs_vfsops.c msdosfs_vnops.c msdosfs_unicode.c

.include <bsd.kmodule.mk>

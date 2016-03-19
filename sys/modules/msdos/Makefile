#	$NetBSD: Makefile,v 1.1.50.1 2016/03/19 11:30:32 skrll Exp $

.include "../Makefile.inc"

.PATH:	${S}/fs/msdosfs

KMOD=	msdos
SRCS=	msdosfs_conv.c msdosfs_denode.c msdosfs_fat.c msdosfs_lookup.c \
	msdosfs_vfsops.c msdosfs_vnops.c msdosfs_unicode.c

.include <bsd.kmodule.mk>

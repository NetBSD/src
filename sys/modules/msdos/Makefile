#	$NetBSD: Makefile,v 1.3 2019/02/17 04:05:54 rin Exp $

.include "../Makefile.inc"

.PATH:	${S}/fs/msdosfs

KMOD=	msdos
SRCS=	msdosfs_conv.c msdosfs_denode.c msdosfs_fat.c msdosfs_lookup.c \
	msdosfs_vfsops.c msdosfs_vnops.c msdosfs_unicode.c

WARNS=	3

.include <bsd.kmodule.mk>

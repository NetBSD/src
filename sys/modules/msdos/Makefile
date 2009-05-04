#	$NetBSD: Makefile,v 1.1.10.2 2009/05/04 08:14:11 yamt Exp $

.include "../Makefile.inc"

.PATH:	${S}/fs/msdosfs

KMOD=	msdos
SRCS=	msdosfs_conv.c msdosfs_denode.c msdosfs_fat.c msdosfs_lookup.c \
	msdosfs_vfsops.c msdosfs_vnops.c

.include <bsd.kmodule.mk>

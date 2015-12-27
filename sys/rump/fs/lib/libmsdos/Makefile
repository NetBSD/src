#	$NetBSD: Makefile,v 1.2.38.1 2015/12/27 12:10:13 skrll Exp $
#

.PATH:  ${.CURDIR}/../../../../fs/msdosfs

LIB=	rumpfs_msdos
COMMENT=FAT

SRCS=	msdosfs_conv.c msdosfs_fat.c msdosfs_vfsops.c msdosfs_denode.c	\
	msdosfs_lookup.c msdosfs_vnops.c

.include <bsd.lib.mk>
.include <bsd.klinks.mk>

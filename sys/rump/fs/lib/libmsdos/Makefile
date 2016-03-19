#	$NetBSD: Makefile,v 1.2.38.2 2016/03/19 11:30:36 skrll Exp $
#

.PATH:  ${.CURDIR}/../../../../fs/msdosfs

LIB=	rumpfs_msdos
COMMENT=FAT

SRCS=	msdosfs_conv.c msdosfs_fat.c msdosfs_vfsops.c msdosfs_denode.c	\
	msdosfs_lookup.c msdosfs_vnops.c msdosfs_unicode.c

.include <bsd.lib.mk>
.include <bsd.klinks.mk>

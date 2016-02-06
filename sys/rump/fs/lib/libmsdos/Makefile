#	$NetBSD: Makefile,v 1.4 2016/02/06 10:40:58 mlelstv Exp $
#

.PATH:  ${.CURDIR}/../../../../fs/msdosfs

LIB=	rumpfs_msdos
COMMENT=FAT

SRCS=	msdosfs_conv.c msdosfs_fat.c msdosfs_vfsops.c msdosfs_denode.c	\
	msdosfs_lookup.c msdosfs_vnops.c msdosfs_unicode.c

.include <bsd.lib.mk>
.include <bsd.klinks.mk>

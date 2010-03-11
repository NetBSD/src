#	$NetBSD: Makefile,v 1.1.18.3 2010/03/11 15:04:36 yamt Exp $
#

.PATH:  ${.CURDIR}/../../../../fs/msdosfs

LIB=	rumpfs_msdos

SRCS=	msdosfs_conv.c msdosfs_fat.c msdosfs_vfsops.c msdosfs_denode.c	\
	msdosfs_lookup.c msdosfs_vnops.c

.include <bsd.lib.mk>
.include <bsd.klinks.mk>

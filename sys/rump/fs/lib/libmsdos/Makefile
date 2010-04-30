#	$NetBSD: Makefile,v 1.1.22.1 2010/04/30 14:44:28 uebayasi Exp $
#

.PATH:  ${.CURDIR}/../../../../fs/msdosfs

LIB=	rumpfs_msdos

SRCS=	msdosfs_conv.c msdosfs_fat.c msdosfs_vfsops.c msdosfs_denode.c	\
	msdosfs_lookup.c msdosfs_vnops.c

.include <bsd.lib.mk>
.include <bsd.klinks.mk>

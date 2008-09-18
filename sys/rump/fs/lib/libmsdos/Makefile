#	$NetBSD: Makefile,v 1.1.2.2 2008/09/18 04:37:03 wrstuden Exp $
#

.PATH:  ${.CURDIR}/../../../../fs/msdosfs

LIB=	rumpfs_msdos

SRCS=	msdosfs_conv.c msdosfs_fat.c msdosfs_vfsops.c msdosfs_denode.c	\
	msdosfs_lookup.c msdosfs_vnops.c

CFLAGS+=-Wno-pointer-sign

.include <bsd.lib.mk>
.include <bsd.klinks.mk>

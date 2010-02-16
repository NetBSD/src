#	$NetBSD: Makefile,v 1.5 2010/02/16 20:34:03 pooka Exp $
#

.PATH:  ${.CURDIR}/../../../../fs/ntfs

LIB=	rumpfs_ntfs

SRCS=	ntfs_compr.c ntfs_conv.c ntfs_ihash.c ntfs_subr.c		\
	ntfs_vfsops.c ntfs_vnops.c

CFLAGS+=-Wno-pointer-sign

.include <bsd.lib.mk>
.include <bsd.klinks.mk>

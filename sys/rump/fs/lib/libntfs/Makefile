#	$NetBSD: Makefile,v 1.3.28.1 2008/09/28 10:41:02 mjf Exp $
#

.PATH:  ${.CURDIR}/../../../../fs/ntfs

LIB=	rumpfs_ntfs

SRCS=	ntfs_compr.c ntfs_conv.c ntfs_ihash.c ntfs_subr.c		\
	ntfs_vfsops.c ntfs_vnops.c

CFLAGS+=-Wno-pointer-sign -D__NetBSD__

.include <bsd.lib.mk>
.include <bsd.klinks.mk>

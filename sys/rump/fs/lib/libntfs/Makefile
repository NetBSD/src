#	$NetBSD: Makefile,v 1.7 2015/10/19 16:16:36 pooka Exp $
#

.PATH:  ${.CURDIR}/../../../../fs/ntfs

LIB=	rumpfs_ntfs
COMMENT=NTFS

SRCS=	ntfs_compr.c ntfs_conv.c ntfs_ihash.c ntfs_subr.c		\
	ntfs_vfsops.c ntfs_vnops.c

.include <bsd.lib.mk>
.include <bsd.klinks.mk>

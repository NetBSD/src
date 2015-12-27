#	$NetBSD: Makefile,v 1.6.38.1 2015/12/27 12:10:14 skrll Exp $
#

.PATH:  ${.CURDIR}/../../../../fs/ntfs

LIB=	rumpfs_ntfs
COMMENT=NTFS

SRCS=	ntfs_compr.c ntfs_conv.c ntfs_ihash.c ntfs_subr.c		\
	ntfs_vfsops.c ntfs_vnops.c

.include <bsd.lib.mk>
.include <bsd.klinks.mk>

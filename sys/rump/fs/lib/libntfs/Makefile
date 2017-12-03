#	$NetBSD: Makefile,v 1.6.20.1 2017/12/03 11:39:12 jdolecek Exp $
#

.PATH:  ${.CURDIR}/../../../../fs/ntfs

LIB=	rumpfs_ntfs
COMMENT=NTFS

SRCS=	ntfs_compr.c ntfs_conv.c ntfs_ihash.c ntfs_subr.c		\
	ntfs_vfsops.c ntfs_vnops.c

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
